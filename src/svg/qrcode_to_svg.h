/**
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * @file qrcode_to_svg.h
 *
 * @brief QR Code to SVG conversion utility.
 *
 * @author Julien F.
 * @date 2026-07-12
 *
 * @details This module provides functions to convert QR Code data into SVG format.
 */


#ifndef QR_CODE_TO_SVG_H
#define QR_CODE_TO_SVG_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

// ===========================================================================
// Definitions
// ===========================================================================
#define QR_SVG_BUF_SIZE (40960)
#define SVG_SCALE       (8) /* pixels per module, define the size of the qr code π*/
#define SVG_MARGIN      (4 * SVG_SCALE) /* quiet zone: 4 modules, define the margin of the qr code π*/

// ===========================================================================
// Public function declaration
// ===========================================================================
/**
 * @brief Render a QR code bit matrix as a pure SVG document.
 *
 * Writes only the SVG markup (no HTTP headers) into @p buf, using a compact
 * <path> encoding for the dark modules. The caller is responsible for
 * wrapping it in an HTTP response.
 *
 * @param qr_code[in]      QR code bit matrix (from qrcodegen_encodeText()).
 * @param qr_code_size[in] Side length of the QR grid, in modules.
 * @param buf[out]         Destination buffer for the SVG document.
 * @param buf_size[in]     Size of @p buf (use @ref QR_SVG_BUF_SIZE).
 *
 * @return Number of bytes written on success, negative on error
 *         (e.g. buffer too small).
 */
int qrcode_to_svg(const uint8_t *qr_code, int qr_code_size, char *buf, size_t buf_size);

#ifdef __cplusplus
}
#endif

#endif /* QR_CODE_TO_SVG_H */
