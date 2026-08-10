// SPDX-License-Identifier: Apache-2.0
//
// Unit tests for the QR Code → SVG conversion (qrcode_to_svg.c / qrcode_to_svg.h).

#include "qr_encode.h"
#include "qrcode_to_svg.h"

extern "C"
{
}

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <gtest/gtest.h>
#include <sstream>
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

/**
 * @brief Read an entire file into a string.
 *
 * @param path  Filesystem path of the file to read.
 * @return The file contents on success, or an empty string on failure (the
 *         caller must also check the @p ok flag).
 */
std::string readFile(const std::string &path, bool *ok)
{
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open())
    {
        if (ok != nullptr)
        {
            *ok = false;
        }
        return std::string();
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    if (ok != nullptr)
    {
        *ok = true;
    }
    return ss.str();
}

/**
 * @brief Encode a share string payload and render the resulting QR grid as SVG.
 *
 * @param share  The share string (e.g. "1:b70a...") to encode.
 * @return The generated SVG document on success, or an empty string on failure.
 */
std::string renderShareSvg(const std::string &share)
{
    // The QR encoder supports alphanumeric mode only, and the reference SVG
    // files on disk were generated from the UPPERCASED share strings (the
    // front-end uppercases the hex payload before requesting the QR code, see
    // split.js). Mirror that behaviour here so the golden comparison matches.
    std::string upper = share;
    for (char &c : upper)
    {
        if (c >= 'a' && c <= 'z')
        {
            c = static_cast<char>(c - 'a' + 'A');
        }
    }

    uint8_t qrcode[kBufLen];
    if (!encodeText(upper, qrcode))
    {
        return std::string();
    }
    const int size = qrcode[0];
    if (size <= 0)
    {
        return std::string();
    }

    char buf[QR_SVG_BUF_SIZE];
    int len = qrcode_to_svg(qrcode, size, buf, sizeof(buf));
    if (len < 0)
    {
        return std::string();
    }
    return std::string(buf, static_cast<size_t>(len));
}

/**
 * @brief Absolute path of the golden reference SVG file for a given share.
 *
 * @param shareIndex  Share number (1..5).
 * @return The path to `seed-phrase-share<shareIndex>.svg` under the reference
 *         directory configured at build time (REFERENCE_SVG_DIR).
 */
std::string referencePath(int shareIndex)
{
    return std::string(REFERENCE_SVG_DIR) + "/seed-phrase-share" + std::to_string(shareIndex) + ".svg";
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
        ASSERT_TRUE(encodeText("HORCRUX CORE", qrcode_));
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
    EXPECT_EQ(tail, "</svg>\n") << "Document must end with the closing </svg> tag";
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
    EXPECT_EQ(std::strstr(buf, cmd), nullptr) << "Light module at (1, 1) should not appear in the SVG";
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
    ASSERT_TRUE(encodeText("A DIFFERENT PAYLOAD", other));
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


// ---------------------------------------------------------------------------
// Real cases tests from svg references
// ---------------------------------------------------------------------------
/*
 * Theses tests are generated from an edge case scenario which is: A 24-words seed phrase
 *
 * Here is the seed phrase:
 * "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon
 * abandon abandon abandon abandon abandon abandon abandon abandon abandon art
 *
 * Shares have been generated under the forms of qrcode using Horcrux-core v1.2.3 on a ESP32-DevkitV1
 * shares are available under reference/svg/seed-phrase-shareX.svg
 */


const char seedphrase_share1[] =
    "1:"
    "55CCCF27E4B36A8104992CD2095F02F9F1600F628F217C34E2767611503F7818AB9CC6573E0A8941E71ED7884E6F0CF50B72EF71C164B497A3"
    "DA27CD7159FE349CE4C00B7254F3F52DF0FD283265B0601138A3A1E7114F58320E9DBD9EA0100B3A2E709182DEF71493647CCD32A98B73AD97"
    "C6BD05AA33A9A955668D95328A11821050597D04E299396AC2267A327D957CA41D0638D0CE152B76645994ACD281AA4B3DEBBCFDAF7B20B80E"
    "B08C67EF6393BC8F7E561D4A2F0689E2";
const char seedphrase_share2[] =
    "2:"
    "FAA770633DED651F75B117275FD9DBCD1D3043B85C6ED1D9FA935DD9EED176C11554081A35FF97E2D40D6E29C4CFF7E8E6542A5FD71513B925"
    "199D84F1E9D5055B4A6FA40E098A981C79A15810070C54628BF14BBCEDD12A59685CFAE4DCA83C3306B6B7B28BC30C60ABFB74A50A84C72A08"
    "22491CC0C5A96AE00702FDB587FE701B8B32856B90D828A84234523F67B2ACF18B5D02FDF92692069D3C2EFA57B2F6950AFC31DC2972D759C0"
    "3F4BE8FAFCD11FE770FFA6817C4B65B0";
const char seedphrase_share3[] =
    "3:"
    "CE09DE2ABD3161BE104A5A9B32E9B7148D322DB4B720C3CD79874AA6DA8160F9DFAAAF236F9A70835271D8CFEECF953D8C44A440721EC90EE7"
    "A1DB27E4DF4511A6CCCEC11832174D50EB3D1E460DD21412D133843F93F0520A04A0291E13D617684AA748543A5A3892ADE6D7F3CC6194E6FD"
    "859A7D059820A2D700E10CE863CF9369BA059C001C6170A0E17C4C627407B137F7355E425913D812980BDE39EB133DBC5679E94EE8299683AF"
    "E1A3E07BBF23C10960CDD4A5732C9E26";
const char seedphrase_share4[] =
    "4:"
    "ADCD003EC5F474DD1C271054BB6DB0EA9E85F0CCED2F51A062D25124CE708E8B4867439E7F2737BFAED68496C28228542356DC30BC30F194A8"
    "D4594B9FFD355E54CE3AE1B3E3846F22C45BDCC1549335C6C46692F693D2F15F134EF9D8CA112B78772D37D7FE0973B6D2F85D52404DEAFAEC"
    "BF7E4AD2671F41E01644D65B4F415ED2B81EDC0250B93AA8C4DF204AC0BE114FE6A9FDA146871B81773A027E6F1E3C35D66B1B14B539C672DF"
    "3987105FF03DDD69AA9852018B889906";
const char seedphrase_share5[] =
    "5:"
    "9963AE774528707C79DC5DE8D65DDC330E879EC0066143B4E1C6465BFA2098B38299E4A72542D0DE28AA3270E8824A814946522F193B2B236A"
    "6C1FE88ACBA54AA9489B84A5D819BA6E56C79A975E4D75B69EA45D75EDF3890C7FB22A22056F00233B3CC8314F904744D4E5FE0486A8B93619"
    "18AD2B173A9689D711A72706AB70BDA08929C569DC0062A067973E17D30B0C899AC1A11EE6B25195720DF2BDD3BFF71C8AEEC386746287A8B0"
    "E76F18DEB3CF0387BAAA202584EF6290";

// ---------------------------------------------------------------------------
// Golden-file comparison tests
// ---------------------------------------------------------------------------
//
// These tests re-encode each seed-phrase share string with the current QR code
// encoder + SVG renderer and compare the resulting document byte-for-byte with
// the golden reference SVG files checked in under tests/reference/svg/. They
// guarantee that the output stays stable and identical to what was generated
// on Horcrux-core v1.2.3 on an ESP32-DevKitV1.

/**
 * @brief Encode a share string, render it to SVG and compare against its
 *        golden reference file.
 *
 * @param share       The share string (e.g. "1:b70a...").
 * @param shareIndex  Share number (1..5), used to locate the reference file.
 */
void expectMatchesReference(const std::string &share, int shareIndex)
{
    // Render the current SVG from the share string.
    const std::string generated = renderShareSvg(share);
    ASSERT_FALSE(generated.empty()) << "Failed to render SVG for share " << shareIndex;

    // Read the golden reference file.
    bool ok = false;
    const std::string reference = readFile(referencePath(shareIndex), &ok);
    ASSERT_TRUE(ok) << "Reference SVG file not found: " << referencePath(shareIndex);

    // The generated document must match the reference byte-for-byte.
    EXPECT_EQ(generated, reference) << "Share " << shareIndex << " SVG does not match the reference";
}

TEST_F(SVGTest, ReferenceTestSeedPhraseShare1)
{
    expectMatchesReference(seedphrase_share1, 1);
}

TEST_F(SVGTest, ReferenceTestSeedPhraseShare3)
{
    expectMatchesReference(seedphrase_share3, 3);
}

TEST_F(SVGTest, ReferenceTestSeedPhraseShare4)
{
    expectMatchesReference(seedphrase_share4, 4);
}

TEST_F(SVGTest, ReferenceTestSeedPhraseShare5)
{
    expectMatchesReference(seedphrase_share5, 5);
}

// ---------------------------------------------------------------------------
// Share 2 has no golden reference file, so only structural checks are done.
// ---------------------------------------------------------------------------

TEST_F(SVGTest, ReferenceTestSeedPhraseShare2Structure)
{
    const std::string generated = renderShareSvg(seedphrase_share2);
    ASSERT_FALSE(generated.empty()) << "Failed to render SVG for share 2";

    // With the alphanumeric-only encoder, the uppercased share 2 payload is
    // encoded as a version-10 QR code: 57 modules, so the image is 57*8 + 64
    // = 520 px, matching the golden reference SVGs for shares 1, 3, 4 and 5.
    EXPECT_NE(generated.find("viewBox=\"0 0 520 520\""), std::string::npos);
    EXPECT_NE(generated.find("width=\"520\" height=\"520\""), std::string::npos);
    EXPECT_NE(generated.find("<rect width=\"100%\" height=\"100%\" fill=\"white\"/>"), std::string::npos);
    EXPECT_NE(generated.find("<path d=\""), std::string::npos);
    EXPECT_NE(generated.find("stroke=\"black\" stroke-width=\"8\"/>"), std::string::npos);
}
