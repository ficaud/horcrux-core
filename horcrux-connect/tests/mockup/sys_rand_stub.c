/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Stub for Zephyr's sys_rand_get() — uses /dev/urandom on Linux.
 *
 * Compile and link this together with shamir.c when building
 * native-Linux unit tests outside the Zephyr build system.
 */

#include <stdio.h>
#include <stdlib.h>

void sys_rand_get(void *dst, size_t len)
{
    FILE *f = fopen("/dev/urandom", "r");
    if (!f)
    {
        abort();
    }
    size_t n = fread(dst, 1, len, f);
    (void)n;
    fclose(f);
}
