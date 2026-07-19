/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Deterministic mock for Zephyr's sys_rand_get() + sss_set_seed().
 *
 * Compile and link this together with the horcrux-connect sss.c when
 * building the WASM demo outside the Zephyr build system.
 */

#include "demo_sss.h"
#include <stdint.h>

/* LCG state  —  fixed seed for reproducibility */
static unsigned int rng_state = 42U;

void sss_set_seed(unsigned int seed)
{
    rng_state = seed;
}

static unsigned int rand_u32(void)
{
    rng_state = rng_state * 1103515245U + 12345U;
    return rng_state;
}

void sys_rand_get(void *dst, size_t len)
{
    uint8_t *buf = (uint8_t *)dst;
    for (size_t i = 0; i < len; i++)
    {
        buf[i] = (uint8_t)(rand_u32() >> 16);
    }
}
