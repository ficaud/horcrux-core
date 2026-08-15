// SPDX-License-Identifier: GPL-3.0-or-later
//
// QR code decoder wrapper around the quirc library.
//
// This file is intentionally portable (no Zephyr dependency) so it can be
// compiled both for the ESP32 firmware and for the WASM demo under Emscripten.
// It is NOT listed in the firmware CMakeLists.txt on purpose: the ESP32-S3
// lacks the memory for image upload/management, so quirc decoding is used only
// by the WASM demo (see demo/Makefile).

#include "qr_decode.h"

#include "quirc.h"

#include <stdlib.h>
#include <string.h>

// ===========================================================================
// One-shot decode
// ===========================================================================
/*
 * Decode every QR code found in a quirc instance, writing the first
 * successfully decoded payload into @p out. Returns the payload length on
 * success, or a negative value if none of the candidate codes decoded.
 */
static int decode_codes(struct quirc *q, char *out, size_t out_size);
// ===========================================================================
// One-shot decode
// ===========================================================================
int qr_decode_gray(const uint8_t *gray, int width, int height, char *out, size_t out_size)
{
    int ret = -1;

    if (gray == NULL || out == NULL || out_size == 0 || width <= 0 || height <= 0)
    {
        goto exit;
    }

    if (width > QR_DECODE_MAX_DIM || height > QR_DECODE_MAX_DIM)
    {
        goto exit;
    }

    struct quirc *q = quirc_new();
    if (q == NULL)
    {
        goto exit;
    }

    if (quirc_resize(q, width, height) < 0)
    {
        goto out;
    }

    int w;
    int h;
    uint8_t *buf = quirc_begin(q, &w, &h);
    if (buf == NULL)
    {
        goto out;
    }

    memcpy(buf, gray, (size_t)width * (size_t)height);

    quirc_end(q);

    ret = decode_codes(q, out, out_size);

out:
    quirc_destroy(q);
    return ret;
exit:
    return ret;
}

// ===========================================================================
// Streaming decode: for memory constrainted targets (e.g. ESP32)
// Not yet implemented in the demo or in ESP32 firmware
// ===========================================================================
struct qr_decode_ctx
{
    struct quirc *q; /* quirc instance (owns the image buffer) */
    int width;
    int height;
};

struct qr_decode_ctx *qr_decode_begin(int width, int height)
{
    if (width <= 0 || height <= 0 || width > QR_DECODE_MAX_DIM || height > QR_DECODE_MAX_DIM)
    {
        return NULL;
    }

    struct qr_decode_ctx *ctx = malloc(sizeof(*ctx));
    if (ctx == NULL)
    {
        return NULL;
    }

    ctx->q = quirc_new();
    if (ctx->q == NULL)
    {
        free(ctx);
        return NULL;
    }

    if (quirc_resize(ctx->q, width, height) < 0)
    {
        quirc_destroy(ctx->q);
        free(ctx);
        return NULL;
    }

    ctx->width = width;
    ctx->height = height;
    return ctx;
}

uint8_t *qr_decode_buffer(struct qr_decode_ctx *ctx)
{
    if (ctx == NULL || ctx->q == NULL)
    {
        return NULL;
    }

    int w;
    int h;
    return quirc_begin(ctx->q, &w, &h);
}

int qr_decode_commit(struct qr_decode_ctx *ctx, char *out, size_t out_size)
{
    if (ctx == NULL || ctx->q == NULL || out == NULL || out_size == 0)
    {
        return -1;
    }

    quirc_end(ctx->q);

    return decode_codes(ctx->q, out, out_size);
}

void qr_decode_destroy(struct qr_decode_ctx *ctx)
{
    if (ctx == NULL)
    {
        return;
    }

    if (ctx->q != NULL)
    {
        quirc_destroy(ctx->q);
    }
    free(ctx);
}


// ===========================================================================
// Static functions definition
// ===========================================================================
static int decode_codes(struct quirc *q, char *out, size_t out_size)
{
    int ret = -1;
    int count = quirc_count(q);

    if (count <= 0)
    {
        goto exit;
    }

    for (int i = 0; i < count; i++)
    {
        struct quirc_code code;
        struct quirc_data data;

        quirc_extract(q, i, &code);

        quirc_decode_error_t err = quirc_decode(&code, &data);
        if (err == QUIRC_ERROR_DATA_ECC)
        {
            /* Try the mirrored variant (ISO 18004:2015) before giving up. */
            quirc_flip(&code);
            err = quirc_decode(&code, &data);
        }

        if (err == QUIRC_SUCCESS)
        {
            if (data.payload_len > 0 && (size_t)data.payload_len < out_size)
            {
                memcpy(out, data.payload, (size_t)data.payload_len);
                out[data.payload_len] = '\0';
                ret = data.payload_len;
                goto exit;
            }
        }
    }

exit:
    return ret;
}
