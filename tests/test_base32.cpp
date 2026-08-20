// SPDX-License-Identifier: GPL-3.0-or-later
//
// Unit tests for the base32 codec (base32.c / base32.h).

#include "base32.h"
#include "golden_shares.h"

extern "C"
{
}

#include <cstdint>
#include <cstring>
#include <gtest/gtest.h>
#include <string>
#include <vector>

// ===========================================================================
// Test helpers
// ===========================================================================

namespace
{

/**
 * @brief Convert a hexadecimal string into bytes.
 *
 * @param hex   Hex string (uppercase or lowercase).
 * @return Decoded bytes.
 */
std::vector<uint8_t> hexToBytes(const std::string &hex)
{
    std::vector<uint8_t> out;
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i + 1 < hex.size(); i += 2)
    {
        char hi = hex[i];
        char lo = hex[i + 1];
        auto nibble = [](char c) -> int
        {
            if (c >= '0' && c <= '9')
            {
                return c - '0';
            }
            if (c >= 'a' && c <= 'f')
            {
                return c - 'a' + 10;
            }
            return c - 'A' + 10;
        };
        out.push_back((uint8_t)((nibble(hi) << 4) | nibble(lo)));
    }
    return out;
}

/**
 * @brief Extract the hex payload (after the "N:" prefix) and decode it.
 */
std::vector<uint8_t> shareHexBytes(const char *share)
{
    const char *colon = std::strchr(share, ':');
    return hexToBytes(colon == nullptr ? share : colon + 1);
}

/**
 * @brief Golden base32 encodings of the seed-phrase shares (187 bytes each),
 *        derived from the hex shares in golden_shares.h.
 */
const char *const golden_base32[] = {
    "KXGM6J7EWNVICBEZFTJASXYC7HYWAD3CR4QXYNHCOZ3BCUB7PAMKXHGGK47AVCKB44PNPCCON4GPKC3S55Y4CZFUS6R5UJ6NOFM7"
    "4NE44TAAW4SU6P2S34H5FAZGLMDACE4KHIPHCFHVQMQOTW6Z5IAQBM5C44ERQLPPOFETMR6M2MVJRNZ23F6GXUC2UM5JVFKWNDMV"
    "GKFBDAQQKBMX2BHCTE4WVQRGPIZH3FL4UQOQMOGQZYKSW5TELGKKZUUBVJFT32547WXXWIFYB2YIYZ7PMOJ3ZD36KYOUULYGRHRA",

    "7KTXAYZ55VSR65NRC4TV7WO3ZUOTAQ5YLRXNDWP2SNO5T3WRO3ARKVAIDI277F7C2QGW4KOEZ736RZSUFJP5OFITXESRTHME6HU5"
    "KBK3JJX2IDQJRKMBY6NBLAIAODCUMKF7CS545XISUWLILT5OJXFIHQZQNNVXWKF4GDDAVP5XJJIKQTDSUCBCJEOMBRNJNLQAOAX5"
    "WWD744A3RMZIK24Q3AUKQQRUKI7WPMVM6GFV2AX57ETJEBU5HQXPUV5S62KQV7BR3QUXFV2ZYA7UX2H27TIR7Z3Q76TIC7CLMWYA",

    "ZYE54KV5GFQ34ECKLKNTF2NXCSGTELNUW4QMHTLZQ5FKNWUBMD457KVPENXZU4EDKJY5RT7OZ6KT3DCEURAHEHWJB3T2DWZH4TPU"
    "KENGZTHMCGBSC5GVB2Z5DZDA3UQUCLITHBB7SPYFECQEUAUR4E6WC5UEVJ2IKQ5FUOESVXTNP46MMGKON7MFTJ6QLGBAULLQBYIM"
    "5BR47E3JXICZYAA4MFYKBYL4JRRHIB5RG73TKXSCLEJ5QEUYBPPDT2YTHW6FM6PJJ3UCTFUDV7Q2HYD3X4R4CCLAZXKKK4ZMTYTA",

    "VXGQAPWF6R2N2HBHCBKLW3NQ5KPIL4GM5UXVDIDC2JISJTTQR2FUQZ2DTZ7SON57V3LIJFWCQIUFII2W3QYLYMHRSSUNIWKLT76T"
    "KXSUZY5ODM7DQRXSFRC33TAVJEZVY3CGNEXWSPJPCXYTJ345RSQRFN4HOLJX277AS45W2L4F2USAJXVPV3F7PZFNEZY7IHQBMRGW"
    "LNHUCXWSXAPNYASQXE5KRRG7EBFMBPQRJ7TKT7NBI2DRXALXHIBH43Y6HQ25M2Y3CS2TTRTS344YOEC76A6522NKTBJADC4ITEDA",

    "TFR2452FFBYHY6O4LXUNMXO4GMHIPHWAAZQUHNHBYZDFX6RATCZYFGPEU4SUFUG6FCVDE4HIQJFICSKGKIXRSOZLENVGYH7IRLF2"
    "KSVJJCNYJJOYDG5G4VWHTKLV4TLVW2PKIXLV5XZYSDD7WIVCEBLPAARTWPGIGFHZAR2E2TS74BEGVC4TMGIYVUVROOUWRHLRDJZH"
    "A2VXBPNAREU4K2O4ABRKAZ4XHYL5GCYMRGNMDII642ZFDFLSBXZL3U5764OIV3WDQZ2GFB5IWDTW6GG6WPHQHB52VIQCLBHPMKIA",
};

const char *const golden_hex[] = {
    seedphrase_share1,
    seedphrase_share2,
    seedphrase_share3,
    seedphrase_share4,
    seedphrase_share5,
};

} // namespace

// ===========================================================================
// Test fixture
// ===========================================================================

class Base32Test : public ::testing::Test
{
  protected:
};

// ===========================================================================
// RFC 4648 test vectors (padding stripped)
// ===========================================================================

TEST_F(Base32Test, Rfc4648Vectors_Encode)
{
    struct
    {
        const char *input;
        const char *expected;
    } cases[] = {
        {"", ""},
        {"f", "MY"},
        {"fo", "MZXQ"},
        {"foo", "MZXW6"},
        {"foob", "MZXW6YQ"},
        {"fooba", "MZXW6YTB"},
        {"foobar", "MZXW6YTBOI"},
    };

    for (const auto &c : cases)
    {
        char out[64];
        size_t in_len = std::strlen(c.input);
        ASSERT_EQ(base32_encode((const uint8_t *)c.input, in_len, out, sizeof(out)), 0);
        EXPECT_STREQ(out, c.expected);
    }
}

TEST_F(Base32Test, Rfc4648Vectors_Decode)
{
    struct
    {
        const char *input;
        const char *expected;
    } cases[] = {
        {"", ""},
        {"MY", "f"},
        {"MZXQ", "fo"},
        {"MZXW6", "foo"},
        {"MZXW6YQ", "foob"},
        {"MZXW6YTB", "fooba"},
        {"MZXW6YTBOI", "foobar"},
    };

    for (const auto &c : cases)
    {
        uint8_t out[64];
        size_t out_len = 0;
        size_t in_len = std::strlen(c.input);
        ASSERT_EQ(base32_decode(c.input, in_len, out, sizeof(out), &out_len), 0);
        ASSERT_EQ(out_len, std::strlen(c.expected));
        EXPECT_EQ(std::memcmp(out, c.expected, out_len), 0);
    }
}

// ===========================================================================
// Round-trip
// ===========================================================================

TEST_F(Base32Test, RoundTrip_Lengths)
{
    const std::vector<size_t> lengths = {0,  1,  2,  3,  4,  5,  6,  7,   8,   9,   10,
                                         15, 16, 31, 32, 33, 63, 64, 127, 128, 129, 187};

    for (size_t len : lengths)
    {
        std::vector<uint8_t> input(len);
        for (size_t i = 0; i < len; i++)
        {
            input[i] = (uint8_t)(i * 7 + 13);
        }

        std::vector<char> encoded(base32_encoded_len(len));
        ASSERT_EQ(base32_encode(input.data(), len, encoded.data(), encoded.size()), 0);

        std::vector<uint8_t> decoded(len > 0 ? len : 1);
        size_t out_len = 0;
        ASSERT_EQ(base32_decode(encoded.data(), std::strlen(encoded.data()), decoded.data(), decoded.size(), &out_len),
                  0);
        ASSERT_EQ(out_len, len);
        EXPECT_EQ(std::memcmp(decoded.data(), input.data(), len), 0);
    }
}

TEST_F(Base32Test, EncodedLen)
{
    EXPECT_EQ(base32_encoded_len(0), 1);
    EXPECT_EQ(base32_encoded_len(1), 3);
    EXPECT_EQ(base32_encoded_len(2), 5);
    EXPECT_EQ(base32_encoded_len(5), 9);
    EXPECT_EQ(base32_encoded_len(187), 301);
}

// ===========================================================================
// Error handling
// ===========================================================================

TEST_F(Base32Test, Encode_OutputTooSmall)
{
    const uint8_t input[] = "hello";
    char out[4];
    EXPECT_NE(base32_encode(input, 5, out, sizeof(out)), 0);
}

TEST_F(Base32Test, Encode_NullOutput)
{
    const uint8_t input[] = "hello";
    EXPECT_NE(base32_encode(input, 5, nullptr, 32), 0);
}

TEST_F(Base32Test, Encode_NullInput)
{
    char out[32];
    EXPECT_NE(base32_encode(nullptr, 5, out, sizeof(out)), 0);
}

TEST_F(Base32Test, Decode_InvalidLength)
{
    uint8_t out[32];
    size_t out_len = 0;

    // Lengths mod 8 == 1, 3, 6 are invalid for unpadded base32.
    EXPECT_NE(base32_decode("A", 1, out, sizeof(out), &out_len), 0);
    EXPECT_NE(base32_decode("ABC", 3, out, sizeof(out), &out_len), 0);
    EXPECT_NE(base32_decode("ABCDEF", 6, out, sizeof(out), &out_len), 0);
}

TEST_F(Base32Test, Decode_ForbiddenChars)
{
    uint8_t out[32];
    size_t out_len = 0;

    const char *bad[] = {"0", "1", "8", "9", "-", "_", "=", " ", "MY=", "MZXW6!", nullptr};
    for (int i = 0; bad[i] != nullptr; i++)
    {
        EXPECT_NE(base32_decode(bad[i], std::strlen(bad[i]), out, sizeof(out), &out_len), 0) << "char: " << bad[i];
    }
}

TEST_F(Base32Test, Decode_OutputTooSmall)
{
    const char *input = "MZXW6YTBOI"; // decodes to 7 bytes
    uint8_t out[4];
    size_t out_len = 0;
    EXPECT_NE(base32_decode(input, std::strlen(input), out, sizeof(out), &out_len), 0);
}

TEST_F(Base32Test, Decode_NullPointers)
{
    uint8_t out[32];
    size_t out_len = 0;
    EXPECT_NE(base32_decode("MZXW6", 5, nullptr, sizeof(out), &out_len), 0);
    EXPECT_NE(base32_decode("MZXW6", 5, out, sizeof(out), nullptr), 0);
    EXPECT_NE(base32_decode(nullptr, 5, out, sizeof(out), &out_len), 0);
}

// ===========================================================================
// Case-insensitive decoding
// ===========================================================================

TEST_F(Base32Test, Decode_Lowercase)
{
    uint8_t upper[32];
    uint8_t lower[32];
    size_t upper_len = 0;
    size_t lower_len = 0;

    ASSERT_EQ(base32_decode("MZXW6YTB", 8, upper, sizeof(upper), &upper_len), 0);
    ASSERT_EQ(base32_decode("mzxw6ytb", 8, lower, sizeof(lower), &lower_len), 0);

    ASSERT_EQ(upper_len, lower_len);
    EXPECT_EQ(std::memcmp(upper, lower, upper_len), 0);
}

// ===========================================================================
// Output charset constraint (QR alphanumeric-safe)
// ===========================================================================

TEST_F(Base32Test, OutputIsAlphanumericSafe)
{
    std::vector<uint8_t> input(187);
    for (size_t i = 0; i < input.size(); i++)
    {
        input[i] = (uint8_t)i;
    }

    std::vector<char> encoded(base32_encoded_len(input.size()));
    ASSERT_EQ(base32_encode(input.data(), input.size(), encoded.data(), encoded.size()), 0);

    for (size_t i = 0; i < std::strlen(encoded.data()); i++)
    {
        char c = encoded[i];
        bool valid = (c >= 'A' && c <= 'Z') || (c >= '2' && c <= '7');
        EXPECT_TRUE(valid) << "unexpected char '" << c << "' at index " << i;
    }
}

// ===========================================================================
// Golden seed-phrase shares
// ===========================================================================

TEST_F(Base32Test, GoldenShares_Encode)
{
    for (int i = 0; i < 5; i++)
    {
        std::vector<uint8_t> bytes = shareHexBytes(golden_hex[i]);

        std::vector<char> encoded(base32_encoded_len(bytes.size()));
        ASSERT_EQ(base32_encode(bytes.data(), bytes.size(), encoded.data(), encoded.size()), 0);

        EXPECT_STREQ(encoded.data(), golden_base32[i]) << "share " << (i + 1);
    }
}

TEST_F(Base32Test, GoldenShares_Decode)
{
    for (int i = 0; i < 5; i++)
    {
        std::vector<uint8_t> expected = shareHexBytes(golden_hex[i]);

        std::vector<uint8_t> decoded(expected.size());
        size_t out_len = 0;
        size_t in_len = std::strlen(golden_base32[i]);
        ASSERT_EQ(base32_decode(golden_base32[i], in_len, decoded.data(), decoded.size(), &out_len), 0);

        ASSERT_EQ(out_len, expected.size());
        EXPECT_EQ(std::memcmp(decoded.data(), expected.data(), out_len), 0) << "share " << (i + 1);
    }
}
