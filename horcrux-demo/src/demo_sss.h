/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Demo-specific SSS header.
 *
 * Includes the reference horcrux-connect sss.h and adds sss_set_seed(),
 * which is specific to the WASM demo (not part of the embedded API).
 */

#ifndef DEMO_SSS_H
#define DEMO_SSS_H

#include "sss.h" /* horcrux-connect sss.h — provides sss_split, sss_combine, etc. */

#ifdef __cplusplus
extern "C"
{
#endif

/** Seed the internal PRNG (called from JavaScript with crypto-secure seed). */
void sss_set_seed(unsigned int seed);

#ifdef __cplusplus
}
#endif

#endif /* DEMO_SSS_H */
