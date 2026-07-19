// SPDX-License-Identifier: Apache-2.0
//
// Google Test entry point.

#include <gtest/gtest.h>

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
