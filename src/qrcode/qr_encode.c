/*
 * QR Code generator library (C) — minimal build tailored for Horcrux.
 *
 * Copyright (c) Project Nayuki. (MIT License)
 * https://www.nayuki.io/page/qr-code-generator-library
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * - The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 * - The Software is provided "as is", without warranty of any kind, express or
 *   implied, including but not limited to the warranties of merchantability,
 *   fitness for a particular purpose and noninfringement. In no event shall the
 *   authors or copyright holders be liable for any claim, damages or other
 *   liability, whether in an action of contract, tort or otherwise, arising from,
 *   out of or in connection with the Software or the use or other dealings in the
 *   Software.
 *
 * This is a reduced build of the upstream Nayuki library tailored to this
 * application. Removed compared to upstream:
 *   - ECI, Kanji, byte and numeric encoding modes (alphanumeric remains)
 *   - the segments / binary mid-level API (encodeSegments*, makeBytes, makeEci...)
 *   - the MEDIUM / QUARTILE / HIGH error correction levels (only LOW is kept)
 *   - the forced-mask option and the ECC boosting
 * The only supported use case is encoding an alphanumeric text string with
 * ECC LOW, mask AUTO and versions 1–40, producing the same bit-packed qrcode[]
 * format as upstream (byte 0 = size, modules from byte 1, row-major, LSB first).
 */

#include "qr_encode.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

/* ===========================================================================
 * Private constants
 * =========================================================================== */

/* The set of legal characters in alphanumeric mode. */
static const char *ALPHANUMERIC_CHARSET = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ $%*+-./:";

/* Sentinel value used by bit-length computations on overflow. */
#define LENGTH_OVERFLOW -1

/* Low error correction level: about 7% of codewords can be restored. */
static const int8_t ECC_CODEWORDS_PER_BLOCK[41] = {
    -1, 7,  10, 15, 20, 26, 18, 20, 24, 30, 18, 20, 24, 26, 30, 22, 24, 28, 30, 28, 28,
    28, 28, 30, 30, 26, 28, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
};
static const int8_t NUM_ERROR_CORRECTION_BLOCKS[41] = {
    -1, 1, 1, 1,  1,  1,  2,  2,  2,  2,  4,  4,  4,  4,  4,  6,  6,  6,  6,  7,  8,
    8,  9, 9, 10, 12, 12, 12, 13, 14, 15, 16, 17, 18, 19, 19, 20, 21, 22, 24, 25,
};

#define REED_SOLOMON_DEGREE_MAX 30 /* Based on the table above */

/* Penalty weights for the automatic mask selection. */
static const int PENALTY_N1 = 3;
static const int PENALTY_N2 = 3;
static const int PENALTY_N3 = 40;
static const int PENALTY_N4 = 10;

/* ===========================================================================
 * Private types
 * =========================================================================== */
/* A single segment of data bits, packed in bitwise big endian. */
typedef struct
{
    int numChars; /* number of alphanumeric characters */
    uint8_t *data;
    int bitLength;
} qrcodegen_Segment;

/* ===========================================================================
 * Private function declarations
 * =========================================================================== */

static void appendBitsToBuffer(unsigned int val, int numBits, uint8_t buffer[], int *bitLen);
static void addEccAndInterleave(uint8_t data[], int version, uint8_t result[]);
static int getNumDataCodewords(int version);
static int getNumRawDataModules(int ver);
static void reedSolomonComputeDivisor(int degree, uint8_t result[]);
static void reedSolomonComputeRemainder(const uint8_t data[],
                                        int dataLen,
                                        const uint8_t generator[],
                                        int degree,
                                        uint8_t result[]);
static uint8_t reedSolomonMultiply(uint8_t x, uint8_t y);
static void initializeFunctionModules(int version, uint8_t qrcode[]);
static void drawLightFunctionModules(uint8_t qrcode[], int version);
static void drawFormatBits(int mask, uint8_t qrcode[]);
static int getAlignmentPatternPositions(int version, uint8_t result[7]);
static void fillRectangle(int left, int top, int width, int height, uint8_t qrcode[]);
static void drawCodewords(const uint8_t data[], int dataLen, uint8_t qrcode[]);
static void applyMask(const uint8_t functionModules[], uint8_t qrcode[], int mask);
static long getPenaltyScore(const uint8_t qrcode[]);
static int finderPenaltyCountPatterns(const int runHistory[7], int qrsize);
static int finderPenaltyTerminateAndCount(bool currentRunColor, int currentRunLength, int runHistory[7], int qrsize);
static void finderPenaltyAddHistory(int currentRunLength, int runHistory[7], int qrsize);
static bool getModuleBounded(const uint8_t qrcode[], int x, int y);
static void setModuleBounded(uint8_t qrcode[], int x, int y, bool isDark);
static void setModuleUnbounded(uint8_t qrcode[], int x, int y, bool isDark);
static bool getBit(int x, int i);
static bool isAlphanumeric(const char *text);
static qrcodegen_Segment makeAlphanumeric(const char *text, uint8_t buf[]);
static int calcSegmentBitLength(size_t numChars);
static int numCharCountBits(int version);

/* ===========================================================================
 * Public functions
 * =========================================================================== */

bool qrcodegen_encodeText(const char *text, uint8_t tempBuffer[], uint8_t qrcode[])
{
    bool ret = false;

    if (text == NULL || tempBuffer == NULL || qrcode == NULL)
    {
        if (qrcode != NULL)
        {
            qrcode[0] = 0; /* Invalid size sentinel */
        }

        goto exit;
    }

    /* Build a single segment in the compact alphanumeric mode. */

    if (!isAlphanumeric(text))
    {
        /* Only alphanumeric mode is supported. */
        qrcode[0] = 0; /* Invalid size sentinel */
        goto exit;
    }

    qrcodegen_Segment seg = makeAlphanumeric(text, tempBuffer);

    if (seg.bitLength == LENGTH_OVERFLOW)
    {
        qrcode[0] = 0;
        goto exit;
    }

    /* Find the smallest version (1..40) that fits the segment at ECC LOW. */
    int version;
    int dataUsedBits;
    for (version = qrcodegen_VERSION_MIN;; version++)
    {
        int dataCapacityBits = getNumDataCodewords(version) * 8;
        dataUsedBits = 4 + numCharCountBits(version) + seg.bitLength;
        if (dataUsedBits <= dataCapacityBits)
        {
            break; /* This version fits the data. */
        }

        if (version >= qrcodegen_VERSION_MAX)
        {
            qrcode[0] = 0;
            goto exit;
        }
    }

    /* Concatenate the mode header and the data bits. */
    memset(qrcode, 0, (size_t)qrcodegen_BUFFER_LEN_FOR_VERSION(version) * sizeof(qrcode[0]));
    int bitLen = 0;
    appendBitsToBuffer(MODE_ALPHANUMERIC, 4, qrcode, &bitLen);
    appendBitsToBuffer((unsigned int)seg.numChars, numCharCountBits(version), qrcode, &bitLen);
    for (int j = 0; j < seg.bitLength; j++)
    {
        int bit = (seg.data[j >> 3] >> (7 - (j & 7))) & 1;
        appendBitsToBuffer((unsigned int)bit, 1, qrcode, &bitLen);
    }

    /* Add the terminator (up to 4 zero bits) and pad to a whole byte. */
    int dataCapacityBits = getNumDataCodewords(version) * 8;
    int terminatorBits = dataCapacityBits - bitLen;
    if (terminatorBits > 4)
    {
        terminatorBits = 4; /* Pad with 4 0s */
    }

    appendBitsToBuffer(0, terminatorBits, qrcode, &bitLen);
    appendBitsToBuffer(0, (8 - bitLen % 8) % 8, qrcode, &bitLen);

    /* Pad with alternating bytes until the data capacity is reached. */
    for (uint8_t padByte = 0xEC; bitLen < dataCapacityBits; padByte ^= 0xEC ^ 0x11)
    {
        appendBitsToBuffer(padByte, 8, qrcode, &bitLen); /* Pad with alternating bytes until data capacity is reached */
    }

    /* Compute the ECC, draw the modules and apply the best mask. */
    addEccAndInterleave(qrcode, version, tempBuffer);
    initializeFunctionModules(version, qrcode);
    drawCodewords(tempBuffer, getNumRawDataModules(version) / 8, qrcode);
    drawLightFunctionModules(qrcode, version);
    initializeFunctionModules(version, tempBuffer);

    /* Automatically choose the mask with the lowest penalty score. */
    int bestMask = 0;
    long minPenalty = LONG_MAX;
    for (int i = 0; i < 8; i++)
    {
        applyMask(tempBuffer, qrcode, i);
        drawFormatBits(i, qrcode);
        long penalty = getPenaltyScore(qrcode);
        if (penalty < minPenalty)
        {
            bestMask = i;
            minPenalty = penalty;
        }
        applyMask(tempBuffer, qrcode, i); /* Undo the mask (XOR twice) */
    }
    applyMask(tempBuffer, qrcode, bestMask); /* Apply the final mask */
    drawFormatBits(bestMask, qrcode); /* Overwrite with the real format bits */
    ret = true;

exit:
    return ret;
}

int qrcodegen_getSize(const uint8_t qrcode[])
{
    if (qrcode == NULL)
    {
        return -1; /* Invalid null pointer */
    }

    return qrcode[0];
}

/* ===========================================================================
 * Private functions
 * =========================================================================== */

/* Appends the given number of low-order bits of val to the bit buffer. */
static void appendBitsToBuffer(unsigned int val, int numBits, uint8_t buffer[], int *bitLen)
{
    for (int i = numBits - 1; i >= 0; i--, (*bitLen)++)
    {
        buffer[*bitLen >> 3] |= ((val >> i) & 1) << (7 - (*bitLen & 7)); /* Set bit buffer[i] = val[i] */
    }
}

/* Computes the ECC of each block and interleaves the result into result[]. */
static void addEccAndInterleave(uint8_t data[], int version, uint8_t result[])
{
    int numBlocks = NUM_ERROR_CORRECTION_BLOCKS[version];
    int blockEccLen = ECC_CODEWORDS_PER_BLOCK[version];
    int rawCodewords = getNumRawDataModules(version) / 8;
    int dataLen = getNumDataCodewords(version);
    int numShortBlocks = numBlocks - rawCodewords % numBlocks;
    int shortBlockDataLen = rawCodewords / numBlocks - blockEccLen;

    uint8_t rsdiv[REED_SOLOMON_DEGREE_MAX];
    reedSolomonComputeDivisor(blockEccLen, rsdiv);
    const uint8_t *dat = data;
    for (int i = 0; i < numBlocks; i++)
    {
        int datLen = shortBlockDataLen + (i < numShortBlocks ? 0 : 1);
        uint8_t *ecc = &data[dataLen]; /* Temporary storage */
        reedSolomonComputeRemainder(dat, datLen, rsdiv, blockEccLen, ecc);
        for (int j = 0, k = i; j < datLen; j++, k += numBlocks)
        {
            if (j == shortBlockDataLen)
            {
                k -= numShortBlocks; /* Skip the short block data */
            }

            result[k] = dat[j];
        }
        for (int j = 0, k = dataLen + i; j < blockEccLen; j++, k += numBlocks)
        {
            result[k] = ecc[j]; /* Copy the ECC */
        }

        dat += datLen;
    }
}

/* Returns the number of 8-bit codewords available for data (not ECC). */
static int getNumDataCodewords(int version)
{
    return getNumRawDataModules(version) / 8 - ECC_CODEWORDS_PER_BLOCK[version] * NUM_ERROR_CORRECTION_BLOCKS[version];
}

/* Returns the number of raw data modules, including remainder bits. */
static int getNumRawDataModules(int ver)
{
    int result = (16 * ver + 128) * ver + 64;
    if (ver >= 2)
    {
        int numAlign = ver / 7 + 2;
        result -= (25 * numAlign - 10) * numAlign - 55;
        if (ver >= 7)
        {
            result -= 36; /* Subtract version*4 if version >= 7 */
        }
    }
    return result;
}

/* Computes a Reed-Solomon ECC generator polynomial for the given degree. */
static void reedSolomonComputeDivisor(int degree, uint8_t result[])
{
    memset(result, 0, (size_t)degree * sizeof(result[0]));
    result[degree - 1] = 1; /* Start off with the monomial x^0 */

    /* Compute (x - r^0) * (x - r^1) * ... * (x - r^{degree-1}) over GF(2^8/0x11D). */
    uint8_t root = 1;
    for (int i = 0; i < degree; i++)
    {
        for (int j = 0; j < degree; j++)
        {
            result[j] = reedSolomonMultiply(result[j], root);
            if (j + 1 < degree)
            {
                result[j] ^= result[j + 1]; /* result[j] = result[j] * x + result[j + 1] */
            }
        }
        root = reedSolomonMultiply(root, 0x02);
    }
}

/* Computes the Reed-Solomon remainder (ECC) of data by the generator polynomial. */
static void reedSolomonComputeRemainder(const uint8_t data[],
                                        int dataLen,
                                        const uint8_t generator[],
                                        int degree,
                                        uint8_t result[])
{
    memset(result, 0, (size_t)degree * sizeof(result[0]));
    for (int i = 0; i < dataLen; i++)
    {
        uint8_t factor = data[i] ^ result[0];
        memmove(&result[0], &result[1], (size_t)(degree - 1) * sizeof(result[0]));
        result[degree - 1] = 0;
        for (int j = 0; j < degree; j++)
        {
            result[j] ^= reedSolomonMultiply(generator[j], factor); /* result[j] = result[j] * factor + gen[j] */
        }
    }
}

/* Returns the product of two field elements modulo GF(2^8/0x11D). */
static uint8_t reedSolomonMultiply(uint8_t x, uint8_t y)
{
    /* Russian peasant multiplication */
    uint8_t z = 0;
    for (int i = 7; i >= 0; i--)
    {
        z = (uint8_t)((z << 1) ^ ((z >> 7) * 0x11D));
        z ^= ((y >> i) & 1) * x;
    }
    return z;
}

/* Clears the grid and marks every function module as dark. */
static void initializeFunctionModules(int version, uint8_t qrcode[])
{
    int qrsize = version * 4 + 17;
    memset(qrcode, 0, (size_t)((qrsize * qrsize + 7) / 8 + 1) * sizeof(qrcode[0]));
    qrcode[0] = (uint8_t)qrsize;

    /* Fill the horizontal and vertical timing patterns. */
    fillRectangle(6, 0, 1, qrsize, qrcode);
    fillRectangle(0, 6, qrsize, 1, qrcode);

    /* Fill the 3 finder patterns (all corners except bottom right) and format bits. */
    fillRectangle(0, 0, 9, 9, qrcode);
    fillRectangle(qrsize - 8, 0, 8, 9, qrcode);
    fillRectangle(0, qrsize - 8, 9, 8, qrcode);

    /* Fill the numerous alignment patterns. */
    uint8_t alignPatPos[7];
    int numAlign = getAlignmentPatternPositions(version, alignPatPos);
    for (int i = 0; i < numAlign; i++)
    {
        for (int j = 0; j < numAlign; j++)
        {
            if (!((i == 0 && j == 0) || (i == 0 && j == numAlign - 1) || (i == numAlign - 1 && j == 0)))
                fillRectangle(alignPatPos[i] - 2, alignPatPos[j] - 2, 5, 5, qrcode);
        }
    }

    /* Fill the version blocks. */
    if (version >= 7)
    {
        fillRectangle(qrsize - 11, 0, 3, 6, qrcode);
        fillRectangle(0, qrsize - 11, 6, 3, qrcode);
    }
}

/* Draws the light function modules (and some dark ones) onto the grid. */
static void drawLightFunctionModules(uint8_t qrcode[], int version)
{
    int qrsize = qrcodegen_getSize(qrcode);

    /* Draw the horizontal and vertical timing patterns. */
    for (int i = 7; i < qrsize - 7; i += 2)
    {
        setModuleBounded(qrcode, 6, i, false);
        setModuleBounded(qrcode, i, 6, false);
    }

    /* Draw the 3 finder patterns (overwrites some timing modules). */
    for (int dy = -4; dy <= 4; dy++)
    {
        for (int dx = -4; dx <= 4; dx++)
        {
            int dist = abs(dx);
            if (abs(dy) > dist)
            {
                dist = abs(dy); /* Dist = abs(dy), with special case dy = 0 handled above */
            }

            if (dist == 2 || dist == 4)
            {
                setModuleUnbounded(qrcode, 3 + dx, 3 + dy, false);
                setModuleUnbounded(qrcode, qrsize - 4 + dx, 3 + dy, false);
                setModuleUnbounded(qrcode, 3 + dx, qrsize - 4 + dy, false);
            }
        }
    }

    /* Draw the numerous alignment patterns. */
    uint8_t alignPatPos[7];
    int numAlign = getAlignmentPatternPositions(version, alignPatPos);
    for (int i = 0; i < numAlign; i++)
    {
        for (int j = 0; j < numAlign; j++)
        {
            if ((i == 0 && j == 0) || (i == 0 && j == numAlign - 1) || (i == numAlign - 1 && j == 0))
            {
                continue; /* Skip the three finder corners */
            }

            for (int dy = -1; dy <= 1; dy++)
            {
                for (int dx = -1; dx <= 1; dx++)
                    setModuleBounded(qrcode, alignPatPos[i] + dx, alignPatPos[j] + dy, dx == 0 && dy == 0);
            }
        }
    }

    /* Draw the version blocks. */
    if (version >= 7)
    {
        int rem = version; /* version is uint6, in the range [7, 40] */
        for (int i = 0; i < 12; i++)
        {
            rem = (rem << 1) ^ ((rem >> 11) * 0x1F25); /* Rem = data ^ ((data << 11) ^ (data << 22)) */
        }

        long bits = (long)version << 12 | rem; /* uint18 */

        /* Draw two copies. */
        for (int i = 0; i < 6; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                int k = qrsize - 11 + j;
                setModuleBounded(qrcode, k, i, (bits & 1) != 0);
                setModuleBounded(qrcode, i, k, (bits & 1) != 0);
                bits >>= 1;
            }
        }
    }
}

/* Draws the two copies of the format bits (with their ECC) for the given mask.
 * The ECC level is fixed to LOW. */
static void drawFormatBits(int mask, uint8_t qrcode[])
{
    /* Format bits: 2-bit ECC level code for LOW is 01, then the 3-bit mask. */
    int data = (1 << 3) | mask; /* errCorrLvl=01 (LOW), mask = 3 bits */
    int rem = data;
    for (int i = 0; i < 10; i++)
    {
        rem = (rem << 1) ^ ((rem >> 9) * 0x537); /* Rem = data ^ ((data << 9) ^ (data << 18)) */
    }

    int bits = (data << 10 | rem) ^ 0x5412; /* uint15 */

    /* Draw the first copy. */
    for (int i = 0; i <= 5; i++)
    {
        setModuleBounded(qrcode, 8, i, getBit(bits, i)); /* Horizontal black/white bars */
    }

    setModuleBounded(qrcode, 8, 7, getBit(bits, 6));
    setModuleBounded(qrcode, 8, 8, getBit(bits, 7));
    setModuleBounded(qrcode, 7, 8, getBit(bits, 8));

    for (int i = 9; i < 15; i++)
    {
        setModuleBounded(qrcode, 14 - i, 8, getBit(bits, i)); /* Vertical black/white bars */
    }

    /* Draw the second copy. */
    int qrsize = qrcodegen_getSize(qrcode);

    for (int i = 0; i < 8; i++)
    {
        setModuleBounded(qrcode, qrsize - 1 - i, 8, getBit(bits, i)); /* Lower right shadows */
    }

    for (int i = 8; i < 15; i++)
    {
        setModuleBounded(qrcode, 8, qrsize - 15 + i, getBit(bits, i)); /* Upper right shadows */
    }

    setModuleBounded(qrcode, 8, qrsize - 8, true); /* Always dark */
}

/* Calculates the ascending list of alignment pattern positions for this version. */
static int getAlignmentPatternPositions(int version, uint8_t result[7])
{
    if (version == 1)
    {
        return 0; /* No alignment patterns */
    }

    int numAlign = version / 7 + 2;
    int step = (version * 8 + numAlign * 3 + 5) / (numAlign * 4 - 4) * 2;

    for (int i = numAlign - 1, pos = version * 4 + 10; i >= 1; i--, pos -= step)
    {
        result[i] = (uint8_t)pos; /* Put the position into the result */
    }
    result[0] = 6;

    return numAlign;
}

/* Sets every module in the given rectangle to dark. */
static void fillRectangle(int left, int top, int width, int height, uint8_t qrcode[])
{
    for (int dy = 0; dy < height; dy++)
    {
        for (int dx = 0; dx < width; dx++)
        {
            setModuleBounded(qrcode, left + dx, top + dy, true); /* Darken the rectangle */
        }
    }
}

/* Draws the raw codewords (data + ECC) onto the grid with the zigzag scan. */
static void drawCodewords(const uint8_t data[], int dataLen, uint8_t qrcode[])
{
    int qrsize = qrcodegen_getSize(qrcode);
    int i = 0; /* Bit index into the data */
    for (int right = qrsize - 1; right >= 1; right -= 2)
    {
        if (right == 6)
        {
            right = 5; /* Skip the rightmost pixel, which is always black */
        }

        for (int vert = 0; vert < qrsize; vert++)
        {
            for (int j = 0; j < 2; j++)
            {
                int x = right - j;
                bool upward = ((right + 1) & 2) == 0;
                int y = upward ? qrsize - 1 - vert : vert;
                if (!getModuleBounded(qrcode, x, y) && i < dataLen * 8)
                {
                    bool dark = getBit(data[i >> 3], 7 - (i & 7));
                    setModuleBounded(qrcode, x, y, dark);
                    i++;
                }
            }
        }
    }
}

/* XORs the codeword modules with the given mask pattern. */
static void applyMask(const uint8_t functionModules[], uint8_t qrcode[], int mask)
{
    int qrsize = qrcodegen_getSize(qrcode);
    for (int y = 0; y < qrsize; y++)
    {
        for (int x = 0; x < qrsize; x++)
        {
            if (getModuleBounded(functionModules, x, y))
            {
                continue; /* Skip the module if it is already set */
            }

            bool invert;
            switch (mask)
            {
                case 0:
                    invert = (x + y) % 2 == 0;
                    break;
                case 1:
                    invert = y % 2 == 0;
                    break;
                case 2:
                    invert = x % 3 == 0;
                    break;
                case 3:
                    invert = (x + y) % 3 == 0;
                    break;
                case 4:
                    invert = (x / 3 + y / 2) % 2 == 0;
                    break;
                case 5:
                    invert = x * y % 2 + x * y % 3 == 0;
                    break;
                case 6:
                    invert = (x * y % 2 + x * y % 3) % 2 == 0;
                    break;
                default:
                    invert = ((x + y) % 2 + x * y % 3) % 2 == 0;
                    break;
            }
            bool val = getModuleBounded(qrcode, x, y);
            setModuleBounded(qrcode, x, y, val ^ invert);
        }
    }
}

/* Returns the penalty score used by the automatic mask selection. */
static long getPenaltyScore(const uint8_t qrcode[])
{
    int qrsize = qrcodegen_getSize(qrcode);
    long result = 0;

    /* Adjacent modules of the same color in a row, and finder-like patterns. */
    for (int y = 0; y < qrsize; y++)
    {
        bool runColor = false;
        int runX = 0;
        int runHistory[7] = {0};
        for (int x = 0; x < qrsize; x++)
        {
            if (getModuleBounded(qrcode, x, y) == runColor)
            {
                runX++;
                if (runX == 5)
                {
                    result += PENALTY_N1;
                }
                else if (runX > 5)
                {
                    /* N1 + N3 */
                    result++;
                }
            }
            else
            {
                finderPenaltyAddHistory(runX, runHistory, qrsize);
                if (!runColor)
                    result += finderPenaltyCountPatterns(runHistory, qrsize) * PENALTY_N3;
                runColor = getModuleBounded(qrcode, x, y);
                runX = 1;
            }
        }
        result += finderPenaltyTerminateAndCount(runColor, runX, runHistory, qrsize) * PENALTY_N3;
    }

    /* Adjacent modules of the same color in a column, and finder-like patterns. */
    for (int x = 0; x < qrsize; x++)
    {
        bool runColor = false;
        int runY = 0;
        int runHistory[7] = {0};
        for (int y = 0; y < qrsize; y++)
        {
            if (getModuleBounded(qrcode, x, y) == runColor)
            {
                runY++;
                if (runY == 5)
                {
                    result += PENALTY_N1;
                }
                else if (runY > 5)
                {
                    /* N1 + N3 */
                    result++;
                }
            }
            else
            {
                finderPenaltyAddHistory(runY, runHistory, qrsize);
                if (!runColor)
                {
                    result += finderPenaltyCountPatterns(runHistory, qrsize) * PENALTY_N3;
                }

                runColor = getModuleBounded(qrcode, x, y);
                runY = 1;
            }
        }
        result += finderPenaltyTerminateAndCount(runColor, runY, runHistory, qrsize) * PENALTY_N3;
    }

    /* 2x2 blocks of modules of the same color. */
    for (int y = 0; y < qrsize - 1; y++)
    {
        for (int x = 0; x < qrsize - 1; x++)
        {
            bool color = getModuleBounded(qrcode, x, y);
            if (color == getModuleBounded(qrcode, x + 1, y) && color == getModuleBounded(qrcode, x, y + 1) &&
                color == getModuleBounded(qrcode, x + 1, y + 1))
            {
                result += PENALTY_N2;
            }
        }
    }

    /* Balance of dark and light modules. */
    int dark = 0;
    for (int y = 0; y < qrsize; y++)
    {
        for (int x = 0; x < qrsize; x++)
        {
            if (getModuleBounded(qrcode, x, y))
            {
                dark++;
            }
        }
    }

    int total = qrsize * qrsize; /* Note that size is odd, so dark/total != 1/2 */
    /* Compute the smallest integer k >= 0 such that (45-5k)% <= dark/total <= (55+5k)% */
    int k = (int)((labs(dark * 20L - total * 10L) + total - 1) / total) - 1;

    if (k < 0 || k > 9)
    {
        return -1; /* Overflow guard */
    }

    result += k * PENALTY_N4;
    return result;
}

/* Counts finder-like patterns; helper for getPenaltyScore(). */
static int finderPenaltyCountPatterns(const int runHistory[7], int qrsize)
{
    int n = runHistory[1];
    (void)qrsize;
    bool core = n > 0 && runHistory[2] == n && runHistory[3] == n * 3 && runHistory[4] == n && runHistory[5] == n;
    /* The maximum QR Code size is 177, hence the dark run length n <= 177. */
    return (core && runHistory[0] >= n * 4 && runHistory[6] >= n ? 1 : 0) +
           (core && runHistory[6] >= n * 4 && runHistory[0] >= n ? 1 : 0);
}

/* Must be called at the end of a line of modules; helper for getPenaltyScore(). */
static int finderPenaltyTerminateAndCount(bool currentRunColor, int currentRunLength, int runHistory[7], int qrsize)
{
    if (currentRunColor)
    {
        finderPenaltyAddHistory(currentRunLength, runHistory, qrsize);
        currentRunLength = 0;
    }
    currentRunLength += qrsize; /* Add the light border to the final run */
    finderPenaltyAddHistory(currentRunLength, runHistory, qrsize);
    return finderPenaltyCountPatterns(runHistory, qrsize);
}

/* Pushes a value to the front of the run history; helper for getPenaltyScore(). */
static void finderPenaltyAddHistory(int currentRunLength, int runHistory[7], int qrsize)
{
    if (runHistory[0] == 0)
    {
        currentRunLength += qrsize; /* Add the light border to the initial run */
    }

    memmove(&runHistory[1], &runHistory[0], 6 * sizeof(runHistory[0]));
    runHistory[0] = currentRunLength;
}

/* Returns the color of the module at (x, y), which must be in bounds. */
static bool getModuleBounded(const uint8_t qrcode[], int x, int y)
{
    int qrsize = qrcode[0];
    if (!(qrcodegen_VERSION_MIN * 4 + 17 <= qrsize && qrsize <= qrcodegen_VERSION_MAX * 4 + 17 && 0 <= x &&
          x < qrsize && 0 <= y && y < qrsize))
    {
        return false;
    }

    int index = y * qrsize + x;

    return getBit(qrcode[(index >> 3) + 1], index & 7);
}

/* Sets the color of the module at (x, y), which must be in bounds. */
static void setModuleBounded(uint8_t qrcode[], int x, int y, bool isDark)
{
    int qrsize = qrcode[0];
    if (!(qrcodegen_VERSION_MIN * 4 + 17 <= qrsize && qrsize <= qrcodegen_VERSION_MAX * 4 + 17 && 0 <= x &&
          x < qrsize && 0 <= y && y < qrsize))
    {
        return;
    }

    int index = y * qrsize + x;
    int bitIndex = index & 7;
    int byteIndex = (index >> 3) + 1;

    if (isDark)
    {
        qrcode[byteIndex] |= (1 << bitIndex);
    }
    else
    {
        qrcode[byteIndex] &= (1 << bitIndex) ^ 0xFF;
    }
}

/* Sets the color of the module at (x, y), doing nothing if out of bounds. */
static void setModuleUnbounded(uint8_t qrcode[], int x, int y, bool isDark)
{
    int qrsize = qrcode[0];
    if (0 <= x && x < qrsize && 0 <= y && y < qrsize)
    {
        setModuleBounded(qrcode, x, y, isDark);
    }
}

/* Returns true iff the i'th bit of x is set to 1. Requires x >= 0 and 0 <= i <= 14. */
static bool getBit(int x, int i)
{
    return ((x >> i) & 1) != 0;
}

/* Returns true iff every character of text is in the alphanumeric set. */
static bool isAlphanumeric(const char *text)
{
    for (; *text != '\0'; text++)
    {
        if (strchr(ALPHANUMERIC_CHARSET, *text) == NULL)
        {
            return false;
        }
    }

    return true;
}

/* Builds an alphanumeric-mode segment for the given text. */
static qrcodegen_Segment makeAlphanumeric(const char *text, uint8_t buf[])
{
    qrcodegen_Segment result;
    size_t len = strlen(text);
    int bitLen = calcSegmentBitLength(len);
    result.numChars = (int)len;
    if (bitLen > 0)
    {
        memset(buf, 0, ((size_t)bitLen + 7) / 8 * sizeof(buf[0])); /* clear buffer */
    }

    result.bitLength = 0;

    unsigned int accumData = 0;
    int accumCount = 0;

    for (; *text != '\0'; text++)
    {
        const char *temp = strchr(ALPHANUMERIC_CHARSET, *text);
        accumData = accumData * 45 + (unsigned int)(temp - ALPHANUMERIC_CHARSET);
        accumCount++;
        if (accumCount == 2)
        {
            appendBitsToBuffer(accumData, 11, buf, &result.bitLength);
            accumData = 0;
            accumCount = 0;
        }
    }

    if (accumCount > 0)
    {
        appendBitsToBuffer(accumData, 6, buf, &result.bitLength);
    }

    result.data = buf;
    return result;
}

/* Returns the number of data bits needed for an alphanumeric segment of numChars characters. */
static int calcSegmentBitLength(size_t numChars)
{
    if (numChars > (unsigned int)INT16_MAX)
    {
        return LENGTH_OVERFLOW; /* Reference implementation fails on this case */
    }

    long result = (long)numChars;
    result = (result * 11 + 1) / 2; /* ceil(11/2 * n) */

    if (result < 0 || result > INT16_MAX)
    {
        return LENGTH_OVERFLOW; /* Reference implementation fails on this case */
    }

    return (int)result;
}

/* Returns the bit width of the character count field for alphanumeric mode and the given version. */
static int numCharCountBits(int version)
{
    int i = (version + 7) / 17;
    static const int temp[] = {9, 11, 13};
    return temp[i];
}
