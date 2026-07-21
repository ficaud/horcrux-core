/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Stub header for Zephyr's <zephyr/random/random.h>.
 * Used when compiling the horcrux-connect sss.c outside Zephyr
 * (Emscripten WASM build).  The implementation is in sys_rand_stub.c.
 */

#ifndef ZEPHYR_RANDOM_RANDOM_H
#define ZEPHYR_RANDOM_RANDOM_H

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

void sys_rand_get(void *dst, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_RANDOM_RANDOM_H */
