// SPDX-License-Identifier: Apache-2.0
//
// Unit tests for Shamir's Secret Sharing (horcrux implementation).

#include "sss.h"

extern "C"
{
#include "hazmat.h"
}

#include <cstring>
#include <gtest/gtest.h>
#include <vector>

// ===========================================================================
// Test fixture
// ===========================================================================

class SSSplitTest : public ::testing::Test
{
  protected:
    // sss_split() calls gf_init_tables() lazily — nothing to set up.
};

// ===========================================================================
// Tests
// ===========================================================================

// ---------------------------------------------------------------------------
// 3-of-5 threshold scheme
// ---------------------------------------------------------------------------
TEST_F(SSSplitTest, SplitCombine_3of5)
{
    const uint8_t secret[] = "MonSecretSuperSecret123!";
    size_t len = sizeof(secret); // include null terminator

    struct sss_share shares[5];
    int ret = sss_split(secret, len, 5, 3, shares);
    ASSERT_EQ(ret, 0) << "sss_split failed";

    // Recombine with 3 shares (1, 3, 5)
    struct sss_share subset[3] = {shares[0], shares[2], shares[4]};
    uint8_t got[sizeof(secret)];
    std::memset(got, 0, sizeof(got));

    ret = sss_combine(subset, 3, got, len);
    ASSERT_EQ(ret, 0) << "sss_combine failed";

    EXPECT_EQ(std::memcmp(secret, got, len), 0) << "Reconstructed secret does not match original";
}

// ---------------------------------------------------------------------------
// 3-of-5 with a different subset (2, 3, 4)
// ---------------------------------------------------------------------------
TEST_F(SSSplitTest, SplitCombine_3of5_DifferentSubset)
{
    const uint8_t secret[] = "AnotherSecret-42!";
    size_t len = sizeof(secret);

    struct sss_share shares[5];
    ASSERT_EQ(sss_split(secret, len, 5, 3, shares), 0);

    struct sss_share subset[3] = {shares[1], shares[2], shares[3]};
    uint8_t got[sizeof(secret)];
    std::memset(got, 0, sizeof(got));

    ASSERT_EQ(sss_combine(subset, 3, got, len), 0);
    EXPECT_EQ(std::memcmp(secret, got, len), 0);
}

// ---------------------------------------------------------------------------
// 2-of-2 (trivial threshold)
// ---------------------------------------------------------------------------
TEST_F(SSSplitTest, TwoOfTwo)
{
    const uint8_t secret[] = "AB";
    size_t len = sizeof(secret);

    struct sss_share shares[2];
    ASSERT_EQ(sss_split(secret, len, 2, 2, shares), 0);

    struct sss_share subset[2] = {shares[0], shares[1]};
    uint8_t got[sizeof(secret)];
    std::memset(got, 0, sizeof(got));

    ASSERT_EQ(sss_combine(subset, 2, got, len), 0);
    EXPECT_EQ(std::memcmp(secret, got, len), 0);
}

// ---------------------------------------------------------------------------
// 5-of-5 (all shares required)
// ---------------------------------------------------------------------------
TEST_F(SSSplitTest, FiveOfFive)
{
    const uint8_t secret[] = "AllOrNothing!";
    size_t len = sizeof(secret);

    struct sss_share shares[5];
    ASSERT_EQ(sss_split(secret, len, 5, 5, shares), 0);

    uint8_t got[sizeof(secret)];
    std::memset(got, 0, sizeof(got));

    ASSERT_EQ(sss_combine(shares, 5, got, len), 0);
    EXPECT_EQ(std::memcmp(secret, got, len), 0);
}

// ---------------------------------------------------------------------------
// Threshold not met — 2 shares instead of 3 → must differ
// ---------------------------------------------------------------------------
TEST_F(SSSplitTest, NotEnoughShares_Fails)
{
    const uint8_t secret[] = "Secret123";
    size_t len = sizeof(secret);

    struct sss_share shares[5];
    ASSERT_EQ(sss_split(secret, len, 5, 3, shares), 0);

    struct sss_share subset[2] = {shares[0], shares[1]};
    uint8_t got[sizeof(secret)];
    std::memset(got, 0, sizeof(got));

    ASSERT_EQ(sss_combine(subset, 2, got, len), 0);

    EXPECT_NE(std::memcmp(secret, got, len), 0) << "Secret should NOT be recoverable with only 2 of 3 shares";
}

// ---------------------------------------------------------------------------
// Various secret sizes
// ---------------------------------------------------------------------------
TEST_F(SSSplitTest, VariousSecretSizes)
{
    struct TestCase
    {
        const char *name;
        size_t size;
    };

    TestCase cases[] = {
        {"1 byte", 1},
        {"16 bytes", 16},
        {"64 bytes", 64},
        {"128 bytes", 128},
        {"256 bytes", 256},
    };

    for (const auto &tc : cases)
    {
        std::vector<uint8_t> secret(tc.size);
        for (size_t i = 0; i < tc.size; i++)
        {
            secret[i] = static_cast<uint8_t>(i & 0xFF);
        }

        struct sss_share shares[3];
        ASSERT_EQ(sss_split(secret.data(), tc.size, 3, 2, shares), 0) << "sss_split failed for size=" << tc.name;

        struct sss_share subset[2] = {shares[0], shares[2]};
        std::vector<uint8_t> got(tc.size, 0);

        ASSERT_EQ(sss_combine(subset, 2, got.data(), tc.size), 0) << "sss_combine failed for size=" << tc.name;

        EXPECT_EQ(std::memcmp(secret.data(), got.data(), tc.size), 0) << "Reconstruction mismatch for size=" << tc.name;
    }
}

// ---------------------------------------------------------------------------
// Error cases — invalid parameters
// ---------------------------------------------------------------------------
TEST_F(SSSplitTest, Split_InvalidParams)
{
    uint8_t secret[] = "test";
    struct sss_share shares[5];

    EXPECT_LT(sss_split(nullptr, 4, 5, 3, shares), 0); // NULL secret
    EXPECT_LT(sss_split(secret, 4, 5, 3, nullptr), 0); // NULL output
    EXPECT_LT(sss_split(secret, 0, 5, 3, shares), 0); // len == 0
    EXPECT_LT(sss_split(secret, SSS_MAX_SECRET_LEN + 1, 5, 3, shares), 0); // too long
    EXPECT_LT(sss_split(secret, 4, 2, 3, shares), 0); // n < k
    EXPECT_LT(sss_split(secret, 4, 5, 1, shares), 0); // k < 2
    EXPECT_LT(sss_split(secret, 4, 256, 3, shares), 0); // n > 255
}

TEST_F(SSSplitTest, Combine_InvalidParams)
{
    uint8_t secret[] = "test";
    struct sss_share shares[3];

    ASSERT_EQ(sss_split(secret, 5, 3, 2, shares), 0);

    EXPECT_LT(sss_combine(nullptr, 2, secret, 5), 0); // NULL shares
    EXPECT_LT(sss_combine(shares, 2, nullptr, 5), 0); // NULL output
    EXPECT_LT(sss_combine(shares, 2, secret, 0), 0); // len == 0
    EXPECT_LT(sss_combine(shares, 2, secret, SSS_MAX_SECRET_LEN + 1), 0); // too long
    EXPECT_LT(sss_combine(shares, 1, secret, 5), 0); // k < 2
}

// ===========================================================================
// Known-answer tests  (deterministic random sequence)
// ===========================================================================

// ---------------------------------------------------------------------------
// 2-of-2 with a 1-byte secret  —  split then reconstruct
// ---------------------------------------------------------------------------
TEST_F(SSSplitTest, KnownAnswer_2of2_1byte)
{
    const uint8_t secret[] = {0x42};
    size_t len = 1;

    struct sss_share shares[2];
    ASSERT_EQ(sss_split(secret, len, 2, 2, shares), 0);

    // Reconstruct
    uint8_t got[1] = {0};
    ASSERT_EQ(sss_combine(shares, 2, got, len), 0);
    EXPECT_EQ(got[0], secret[0]);
}

// ---------------------------------------------------------------------------
// 2-of-2 with a 2-byte secret
// ---------------------------------------------------------------------------
TEST_F(SSSplitTest, KnownAnswer_2of2_2bytes)
{
    const uint8_t secret[] = {0x42, 0x43};
    size_t len = 2;

    struct sss_share shares[2];
    ASSERT_EQ(sss_split(secret, len, 2, 2, shares), 0);

    // Reconstruct
    uint8_t got[2] = {0};
    ASSERT_EQ(sss_combine(shares, 2, got, len), 0);
    EXPECT_EQ(got[0], secret[0]);
    EXPECT_EQ(got[1], secret[1]);
}

// ---------------------------------------------------------------------------
// 3-of-5 with a 1-byte secret  —  all C(5,3)=10 subsets reconstruct
// ---------------------------------------------------------------------------
TEST_F(SSSplitTest, KnownAnswer_3of5_AllSubsets)
{
    const uint8_t secret[] = {0x42};
    size_t len = 1;

    struct sss_share shares[5];
    ASSERT_EQ(sss_split(secret, len, 5, 3, shares), 0);

    // All 10 combinations of 3 share indices out of 5
    int combos[10][3] = {
        {0, 1, 2},
        {0, 1, 3},
        {0, 1, 4},
        {0, 2, 3},
        {0, 2, 4},
        {0, 3, 4},
        {1, 2, 3},
        {1, 2, 4},
        {1, 3, 4},
        {2, 3, 4},
    };
    for (int c = 0; c < 10; c++)
    {
        struct sss_share sub[3] = {shares[combos[c][0]], shares[combos[c][1]], shares[combos[c][2]]};
        uint8_t got[1] = {0};
        ASSERT_EQ(sss_combine(sub, 3, got, len), 0);
        EXPECT_EQ(got[0], secret[0]) << "subset [" << combos[c][0] << "," << combos[c][1] << "," << combos[c][2] << "]";
    }
}

// ===========================================================================
// Cross-validation with dsprenkels/sss (hazmat API)
// ===========================================================================

/**
 * @brief Convert our sss_share to sss_Keyshare format.
 *
 * sss_Keyshare is uint8_t[33] where:
 *   [0] = x-coordinate
 *   [1..32] = share data (32 bytes)
 */
static void to_sss_keyshare(sss_Keyshare out, const struct sss_share *in)
{
    out[0] = in->x;
    std::memcpy(out + 1, in->data, 32);
}

/**
 * @brief Convert sss_Keyshare to our sss_share format.
 */
static void from_sss_keyshare(struct sss_share *out, const sss_Keyshare in)
{
    out->x = in[0];
    out->len = 32;
    std::memcpy(out->data, in + 1, 32);
}

// ---------------------------------------------------------------------------
// Test A:  horcrux split  →  sss_combine_keyshares
// ---------------------------------------------------------------------------
TEST_F(SSSplitTest, HorcruxSplit_SssCombine)
{
    uint8_t key[32];
    std::memset(key, 0x42, 32); // deterministic 32-byte secret

    struct sss_share shares[5];
    ASSERT_EQ(sss_split(key, 32, 5, 3, shares), 0);

    // Convert shares 0,2,4 to sss_Keyshare format
    sss_Keyshare ks[3];
    to_sss_keyshare(ks[0], &shares[0]);
    to_sss_keyshare(ks[1], &shares[2]);
    to_sss_keyshare(ks[2], &shares[4]);

    // Combine using sss
    uint8_t restored[32];
    sss_combine_keyshares(restored, ks, 3);

    EXPECT_EQ(std::memcmp(key, restored, 32), 0) << "sss could not reconstruct horcrux shares";
}

// ---------------------------------------------------------------------------
// Test B:  sss_create_keyshares  →  horcrux combine
// ---------------------------------------------------------------------------
TEST_F(SSSplitTest, SssCreate_HorcruxCombine)
{
    uint8_t key[32];
    std::memset(key, 0x43, 32);

    // Split with sss
    sss_Keyshare ks[5];
    sss_create_keyshares(ks, key, 5, 3);

    // Convert shares 1,3,4 to horcrux format
    struct sss_share shares[3];
    from_sss_keyshare(&shares[0], ks[1]);
    from_sss_keyshare(&shares[1], ks[3]);
    from_sss_keyshare(&shares[2], ks[4]);

    // Combine with horcrux
    uint8_t restored[32];
    std::memset(restored, 0, 32);
    ASSERT_EQ(sss_combine(shares, 3, restored, 32), 0);

    EXPECT_EQ(std::memcmp(key, restored, 32), 0) << "horcrux could not reconstruct sss shares";
}

// ---------------------------------------------------------------------------
// Test C:  All C(5,3) subsets — cross-validate each
// ---------------------------------------------------------------------------
TEST_F(SSSplitTest, SssCrossValidation_AllSubsets)
{
    uint8_t key[32];
    std::memset(key, 0x44, 32);

    // Split with horcrux
    struct sss_share shares[5];
    ASSERT_EQ(sss_split(key, 32, 5, 3, shares), 0);

    int combos[10][3] = {
        {0, 1, 2},
        {0, 1, 3},
        {0, 1, 4},
        {0, 2, 3},
        {0, 2, 4},
        {0, 3, 4},
        {1, 2, 3},
        {1, 2, 4},
        {1, 3, 4},
        {2, 3, 4},
    };

    for (int c = 0; c < 10; c++)
    {
        // horcrux self-combine
        struct sss_share sub[3] = {shares[combos[c][0]], shares[combos[c][1]], shares[combos[c][2]]};
        uint8_t got1[32];
        std::memset(got1, 0, 32);
        ASSERT_EQ(sss_combine(sub, 3, got1, 32), 0);
        EXPECT_EQ(std::memcmp(key, got1, 32), 0)
            << "horcrux self-combine failed [" << combos[c][0] << "," << combos[c][1] << "," << combos[c][2] << "]";

        // sss cross-combine
        sss_Keyshare ks[3];
        for (int i = 0; i < 3; i++)
            to_sss_keyshare(ks[i], &shares[combos[c][i]]);

        uint8_t got2[32];
        sss_combine_keyshares(got2, ks, 3);
        EXPECT_EQ(std::memcmp(key, got2, 32), 0)
            << "sss cross-combine failed [" << combos[c][0] << "," << combos[c][1] << "," << combos[c][2] << "]";
    }
}
