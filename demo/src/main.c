/**
 * @file main.c
 *
 * @brief WASM entry point — exposes SSS functions to JavaScript via Emscripten.
 *
 * @author Julien F.
 * @date 2026-07-19
 *
 * @details Compile with:
 *   emcc -O3 -s
 * EXPORTED_FUNCTIONS=_sss_split,_sss_combine,_sss_set_seed,_malloc,_free -s
 * EXPORTED_RUNTIME_METHODS=ccall,cwrap,getValue,setValue -o sss.js main.c sss.c
 *
 * Uses the reference sss.c from horcrux-connect directly (symbolic link / cmake).
 */

#include "demo_sss.h"
#include <emscripten.h>

/* ── JS-callable wrappers (C names exposed via EXPORTED_FUNCTIONS) ── */

void EMSCRIPTEN_KEEPALIVE wasm_set_seed(unsigned int seed) {
  sss_set_seed(seed);
}

/**
 * @brief Split a secret into shares.
 *
 * @param secret       Pointer to secret bytes.
 * @param secret_len   Secret length.
 * @param n            Number of shares.
 * @param k            Threshold.
 * @param shares_out   Pointer to pre-allocated n × sizeof(sss_share) buffer.
 * @return 0 on success, negative on error.
 */
int EMSCRIPTEN_KEEPALIVE sss_split_wasm(const uint8_t *secret,
                                        size_t secret_len, unsigned int n,
                                        unsigned int k,
                                        struct sss_share *shares_out) {
  return sss_split(secret, secret_len, n, k, shares_out);
}

/**
 * @brief Reconstruct a secret from shares.
 *
 * @param shares      Pointer to k shares.
 * @param k           Number of shares (threshold).
 * @param secret_out  Output buffer.
 * @param secret_len  Expected secret length.
 * @return 0 on success, negative on error.
 */
int EMSCRIPTEN_KEEPALIVE sss_combine_wasm(const struct sss_share *shares,
                                          unsigned int k, uint8_t *secret_out,
                                          size_t secret_len) {
  return sss_combine(shares, k, secret_out, secret_len);
}
