/**
 * @file qrcode_to_svg.c
 *
 * @brief QR Code to SVG conversion utility.
 *
 * @author Julien F.
 * @date 2026-07-12
 *
 * @details This module provides functions to convert QR Code data into SVG format.
 */

#include "qrcode_to_svg.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// ===========================================================================
// Zephyr logging module registration
// ===========================================================================

// ===========================================================================
// Structure and variables definition
// ===========================================================================

// ===========================================================================
// Static function declarations
// ===========================================================================
/**
 * @brief Write the opening of a generic SVG document.
 *
 * Writes the XML declaration and the <svg> root element (with viewBox,
 * width and height) into @p buf. The caller must later append the SVG
 * content and the closing </svg> tag.
 *
 * @param buf[out]     Destination buffer for the SVG opening.
 * @param buf_size[in] Size of @p buf.
 * @param width[in]    Canvas width (also used for the viewBox).
 * @param height[in]   Canvas height (also used for the viewBox).
 *
 * @return Number of bytes written on success, negative on error
 *         (e.g. buffer too small).
 */
static int svg_begin(char *buf, size_t buf_size, int width, int height);
// ===========================================================================
// Public function definition
// ===========================================================================
int qrcode_to_svg(const uint8_t *qr_code, int qr_code_size, char *buf, size_t buf_size)
{
    /* Validate inputs: a non-null QR grid, a non-null destination buffer,
     * a positive grid side length and a non-empty buffer. */
    if (qr_code == NULL || buf == NULL || qr_code_size <= 0 || buf_size == 0)
    {
        return -1;
    }

    int img_size = qr_code_size * SVG_SCALE + 2 * SVG_MARGIN;
    bool truncated = false;
    char *p = buf;
    size_t room = buf_size;

    /* SVG opening (XML declaration + <svg> root element) */
    int n = svg_begin(p, room, img_size, img_size);
    if (n < 0 || (size_t)n >= room)
    {
        truncated = true;
    }
    else
    {
        p += n;
        room -= n;
    }

    /* White background (quiet zone) + start of the modules path */
    if (!truncated)
    {
        n = snprintf(p,
                     room,
                     "<rect width=\"100%%\" height=\"100%%\" fill=\"white\"/>\n"
                     "<path d=\"");
        if (n < 0 || (size_t)n >= room)
        {
            truncated = true;
        }
        else
        {
            p += n;
            room -= n;
        }
    }

    /* Draw dark modules as compact path commands.
     * Format: M{x},{y}h{s}  (~13 bytes per dark module vs ~55 for <rect>) */
    if (!truncated)
    {
        for (int y = 0; y < qr_code_size && !truncated; y++)
        {
            for (int x = 0; x < qr_code_size && !truncated; x++)
            {
                int index = y * qr_code_size + x;

                /* Bit-by-bit scan: determine whether the module (x, y) is dark.
                 * The module grid is packed row-major, 8 bits per byte, LSB-first.
                 *   - index >> 3  : index / 8 -> which byte holds this module
                 *   - +1          : skip byte 0 (the version/size header)
                 *   - index & 7   : index % 8 -> which bit inside that byte
                 *   - >> then & 1 : shift the bit to LSB and mask it -> 1 if dark */
                bool dark = ((qr_code[(index >> 3) + 1] >> (index & 7)) & 1) != 0;
                if (dark)
                {
                    n = snprintf(p,
                                 room,
                                 "M%d,%dh%d",
                                 SVG_MARGIN + x * SVG_SCALE,
                                 SVG_MARGIN + y * SVG_SCALE + SVG_SCALE / 2,
                                 SVG_SCALE);
                    if (n < 0 || (size_t)n >= room)
                    {
                        truncated = true;
                    }
                    else
                    {
                        p += n;
                        room -= n;
                    }
                }
            }
        }
    }

    /* Close path and SVG */
    if (!truncated)
    {
        n = snprintf(p, room, "\" stroke=\"black\" stroke-width=\"%d\"/>\n</svg>\n", SVG_SCALE);
        if (n < 0 || (size_t)n >= room)
        {
            truncated = true;
        }
        else
        {
            p += n;
            room -= n;
        }
    }

    if (truncated)
    {
        return -1;
    }

    /* Return the length of the generated SVG document */
    return (int)(p - buf);
}

// ===========================================================================
// Static function definition
// ===========================================================================
static int svg_begin(char *buf, size_t buf_size, int width, int height)
{
    int n = snprintf(buf,
                     buf_size,
                     "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                     "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\""
                     " viewBox=\"0 0 %d %d\" width=\"%d\" height=\"%d\">\n",
                     width,
                     height,
                     width,
                     height);
    if (n < 0 || (size_t)n >= buf_size)
    {
        return -1;
    }

    return n;
}
