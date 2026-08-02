// SPDX-License-Identifier: Apache-2.0
//
// Unit tests for the QR Code → SVG conversion (qrcode_to_svg.c / qrcode_to_svg.h).

#include "qrcode_to_svg.h"
#include "qr_encode.h"

extern "C"
{
}

#include <cstring>
#include <gtest/gtest.h>
#include <string>

// ===========================================================================
// Test helpers
// ===========================================================================

namespace
{

// Buffer large enough for any QR Code (version 40).
constexpr size_t kBufLen = qrcodegen_BUFFER_LEN_MAX;

/**
 * @brief Encode text and return the QR Code buffer.
 *
 * @param text  UTF-8 text to encode.
 * @return true on success, false on failure.
 */
bool encodeText(const std::string &text, uint8_t qrcode[kBufLen])
{
    uint8_t temp[kBufLen];
    return qrcodegen_encodeText(text.c_str(), temp, qrcode);
}

/**
 * @brief Read a single module (pixel) of the QR Code grid.
 *
 * Mirrors the internal getModuleBounded() layout: qrcode[0] holds the side
 * length, the module grid is packed from byte 1, row-major, LSB first.
 *
 * @param qrcode  QR Code buffer produced by qrcodegen_encodeText().
 * @param x       Column (0 = left).
 * @param y       Row (0 = top).
 * @return true if the module is dark, false if light or out of bounds.
 */
bool getModule(const uint8_t qrcode[], int x, int y)
{
    int size = qrcode[0];
    if (x < 0 || y < 0 || x >= size || y >= size)
    {
        return false;
    }
    int index = y * size + x;
    return ((qrcode[(index >> 3) + 1] >> (index & 7)) & 1) != 0;
}

} // namespace

// ===========================================================================
// Test fixture
// ===========================================================================

class SVGTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        // Encode a fixed, non-trivial payload so every test has a valid QR grid.
        ASSERT_TRUE(encodeText("Horcrux Core", qrcode_));
        size_ = qrcode_[0];
        ASSERT_GT(size_, 0);
    }

    uint8_t qrcode_[kBufLen];
    int size_ = 0;
};

// ===========================================================================
// Tests
// ===========================================================================

// ---------------------------------------------------------------------------
// Basic structure
// ---------------------------------------------------------------------------

TEST_F(SVGTest, ReturnsPositiveLength)
{
    char buf[QR_SVG_BUF_SIZE];
    int len = qrcode_to_svg(qrcode_, size_, buf, sizeof(buf));
    EXPECT_GT(len, 0) << "SVG generation should succeed with a large-enough buffer";
}

TEST_F(SVGTest, StartsWithXmlDeclaration)
{
    char buf[QR_SVG_BUF_SIZE];
    int len = qrcode_to_svg(qrcode_, size_, buf, sizeof(buf));
    ASSERT_GT(len, 0);
    EXPECT_STREQ(std::string(buf, strlen("<?xml version=\"1.0\" encoding=\"UTF-8\"?>")).c_str(),
                 "<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
}

TEST_F(SVGTest, ContainsSvgRootAndViewBox)
{
    char buf[QR_SVG_BUF_SIZE];
    qrcode_to_svg(qrcode_, size_, buf, sizeof(buf));

    int img_size = size_ * SVG_SCALE + 2 * SVG_MARGIN;
    EXPECT_NE(std::strstr(buf, "<svg"), nullptr);
    EXPECT_NE(std::strstr(buf, "xmlns=\"http://www.w3.org/2000/svg\""), nullptr);
    EXPECT_NE(std::strstr(buf, "viewBox=\"0 0 "), nullptr);

    // The viewBox must match the computed image size.
    char expected[64];
    snprintf(expected, sizeof(expected), "viewBox=\"0 0 %d %d\"", img_size, img_size);
    EXPECT_NE(std::strstr(buf, expected), nullptr);
}

TEST_F(SVGTest, EndsWithClosingSvgTag)
{
    char buf[QR_SVG_BUF_SIZE];
    int len = qrcode_to_svg(qrcode_, size_, buf, sizeof(buf));
    ASSERT_GT(len, 0);
    // The document must end with the closing </svg> tag (optionally followed
    // by a trailing newline).
    const char *closing = "</svg>";
    EXPECT_GT(len, (int)strlen(closing));
    // Look at the tail of the document, stripping an optional trailing newline.
    std::string tail(buf + len - strlen(closing) - 1);
    EXPECT_EQ(tail, "</svg>\n")
        << "Document must end with the closing </svg> tag";
}

// ---------------------------------------------------------------------------
// Content
// ---------------------------------------------------------------------------

TEST_F(SVGTest, ContainsWhiteBackgroundRect)
{
    char buf[QR_SVG_BUF_SIZE];
    qrcode_to_svg(qrcode_, size_, buf, sizeof(buf));
    EXPECT_NE(std::strstr(buf, "<rect width=\"100%\" height=\"100%\" fill=\"white\"/>"), nullptr);
}

TEST_F(SVGTest, ContainsPathForDarkModules)
{
    char buf[QR_SVG_BUF_SIZE];
    qrcode_to_svg(qrcode_, size_, buf, sizeof(buf));
    EXPECT_NE(std::strstr(buf, "<path d=\""), nullptr);
}

TEST_F(SVGTest, DarkModulesAreRenderedAsPathCommands)
{
    char buf[QR_SVG_BUF_SIZE];
    qrcode_to_svg(qrcode_, size_, buf, sizeof(buf));

    // Every dark module must appear as a path command M{x},{y}h{scale}.
    // Scan the grid and verify each dark module yields a matching command.
    for (int y = 0; y < size_; y++)
    {
        for (int x = 0; x < size_; x++)
        {
            if (getModule(qrcode_, x, y))
            {
                char cmd[64];
                snprintf(cmd,
                         sizeof(cmd),
                         "M%d,%dh%d",
                         SVG_MARGIN + x * SVG_SCALE,
                         SVG_MARGIN + y * SVG_SCALE + SVG_SCALE / 2,
                         SVG_SCALE);
                EXPECT_NE(std::strstr(buf, cmd), nullptr)
                    << "Dark module at (" << x << ", " << y << ") missing from SVG";
            }
        }
    }
}

TEST_F(SVGTest, LightModulesAreNotRendered)
{
    char buf[QR_SVG_BUF_SIZE];
    qrcode_to_svg(qrcode_, size_, buf, sizeof(buf));

    // Verify that a module known to be light (e.g. the separator ring of the
    // top-left finder pattern) does NOT produce a path command.
    // (1,1) is part of the light separator ring of the top-left finder pattern.
    ASSERT_FALSE(getModule(qrcode_, 1, 1));
    char cmd[64];
    snprintf(cmd,
             sizeof(cmd),
             "M%d,%dh%d",
             SVG_MARGIN + 1 * SVG_SCALE,
             SVG_MARGIN + 1 * SVG_SCALE + SVG_SCALE / 2,
             SVG_SCALE);
    EXPECT_EQ(std::strstr(buf, cmd), nullptr)
        << "Light module at (1, 1) should not appear in the SVG";
}

// ---------------------------------------------------------------------------
// Return value / length
// ---------------------------------------------------------------------------

TEST_F(SVGTest, ReturnLengthMatchesStringLength)
{
    char buf[QR_SVG_BUF_SIZE];
    int len = qrcode_to_svg(qrcode_, size_, buf, sizeof(buf));
    ASSERT_GT(len, 0);
    EXPECT_EQ(len, (int)strlen(buf)) << "Returned length must match the actual string length";
}

// ---------------------------------------------------------------------------
// Determinism
// ---------------------------------------------------------------------------

TEST_F(SVGTest, SameInputProducesSameOutput)
{
    char buf1[QR_SVG_BUF_SIZE];
    char buf2[QR_SVG_BUF_SIZE];
    int len1 = qrcode_to_svg(qrcode_, size_, buf1, sizeof(buf1));
    int len2 = qrcode_to_svg(qrcode_, size_, buf2, sizeof(buf2));
    ASSERT_EQ(len1, len2);
    EXPECT_EQ(std::memcmp(buf1, buf2, len1), 0) << "Same QR grid must produce identical SVG";
}

TEST_F(SVGTest, DifferentInputProducesDifferentOutput)
{
    uint8_t other[kBufLen];
    ASSERT_TRUE(encodeText("a different payload", other));
    int otherSize = other[0];
    ASSERT_GT(otherSize, 0);

    char buf1[QR_SVG_BUF_SIZE];
    char buf2[QR_SVG_BUF_SIZE];
    int len1 = qrcode_to_svg(qrcode_, size_, buf1, sizeof(buf1));
    int len2 = qrcode_to_svg(other, otherSize, buf2, sizeof(buf2));
    ASSERT_GT(len1, 0);
    ASSERT_GT(len2, 0);
    EXPECT_NE(std::memcmp(buf1, buf2, std::min(len1, len2)), 0)
        << "Different QR grids should produce different SVG documents";
}

// ---------------------------------------------------------------------------
// Error cases
// ---------------------------------------------------------------------------

TEST_F(SVGTest, BufferTooSmallReturnsMinusOne)
{
    // A buffer too small for even the SVG header.
    char tiny[8];
    EXPECT_LT(qrcode_to_svg(qrcode_, size_, tiny, sizeof(tiny)), 0);
}

TEST_F(SVGTest, NullQrCodeFails)
{
    char buf[QR_SVG_BUF_SIZE];
    EXPECT_LT(qrcode_to_svg(nullptr, size_, buf, sizeof(buf)), 0);
}

TEST_F(SVGTest, NullBufferFails)
{
    EXPECT_LT(qrcode_to_svg(qrcode_, size_, nullptr, QR_SVG_BUF_SIZE), 0);
}

TEST_F(SVGTest, ZeroSizeFails)
{
    char buf[QR_SVG_BUF_SIZE];
    EXPECT_LT(qrcode_to_svg(qrcode_, 0, buf, sizeof(buf)), 0);
}

TEST_F(SVGTest, ZeroLengthBufferFails)
{
    char buf[QR_SVG_BUF_SIZE];
    EXPECT_LT(qrcode_to_svg(qrcode_, size_, buf, 0), 0);
}
