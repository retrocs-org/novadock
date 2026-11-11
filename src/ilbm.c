#include "nd_ilbm.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static unsigned short be16(const unsigned char *p)
{
    return (unsigned short)(((unsigned int)p[0] << 8) | p[1]);
}
static unsigned long be32(const unsigned char *p)
{
    return ((unsigned long)p[0] << 24) | ((unsigned long)p[1] << 16) |
           ((unsigned long)p[2] << 8) | p[3];
}
/* Decode one ILBM planar row; reading through the decompressed scanline
 * boundaries also ensures malformed RLE never bleeds into another row. */
static int decode_row(const unsigned char *src, unsigned long size,
                      unsigned long *pos, unsigned char *dst,
                      unsigned int n, int compression)
{
    unsigned int out = 0;
    while (out < n) {
        if (compression == 0) {
            if (size - *pos < n - out) return 0;
            memcpy(dst + out, src + *pos, n - out);
            *pos += n - out;
            return 1;
        } else {
            signed char ctl;
            unsigned int run;
            if (*pos >= size) return 0;
            ctl = (signed char)src[(*pos)++];
            if (ctl >= 0) {
                run = (unsigned int)ctl + 1;
                if (run > n - out || run > size - *pos) return 0;
                memcpy(dst + out, src + *pos, run);
                *pos += run;
                out += run;
            } else if (ctl != -128) {
                run = (unsigned int)(1 - ctl);
                if (run > n - out || *pos >= size) return 0;
                memset(dst + out, src[(*pos)++], run);
                out += run;
            }
        }
    }
    return 1;
}
int nd_ilbm_load(const char *filename, struct NDILBM *icon)
{
    FILE *fp;
    unsigned char *buf = NULL;
    unsigned char header[12];
    unsigned char plane[ND_ICON_MAX_DIM / 8 * 4 * 4];
    unsigned long form_len, length, pos, body_pos = 0, body_len = 0;
    unsigned int w = 0, h = 0, planes = 0, rowbytes = 0, x, y, p;
    int compression = 0, masking = 0, has_bmhd = 0, has_cmap = 0;
    int success = 0;
    if (!filename || !icon) return 0;
    memset(icon, 0, sizeof(*icon));
    icon->transparent = -1;
    fp = fopen(filename, "rb");
    if (!fp) return 0;
    if (fread(header, 1, 12, fp) != 12 ||
        memcmp(header, "FORM", 4) || memcmp(header + 8, "ILBM", 4)) goto done;
    form_len = be32(header + 4);
    if (form_len < 4 || form_len > 131072UL) goto done;
    length = form_len - 4;
    buf = (unsigned char *)malloc((size_t)length);
    if (!buf || fread(buf, 1, (size_t)length, fp) != length) goto done;
    pos = 0;
    while (pos + 8 <= length) {
        unsigned long chunk_len = be32(buf + pos + 4);
        const unsigned char *data = buf + pos + 8;
        if (chunk_len > length - pos - 8) goto done;
        if (!memcmp(buf + pos, "BMHD", 4)) {
            if (chunk_len < 20) goto done;
            w = be16(data); h = be16(data + 2);
            planes = data[8]; masking = data[9]; compression = data[10];
            if (!w || !h || w > ND_ICON_MAX_DIM || h > ND_ICON_MAX_DIM ||
                planes < 1 || planes > 4 || masking > 2 || masking == 1 ||
                compression > 1) goto done;
            icon->transparent = masking == 2 ? (short)be16(data + 12) : -1;
            has_bmhd = 1;
        } else if (!memcmp(buf + pos, "CMAP", 4)) {
            unsigned int i, cnt = (unsigned int)(chunk_len / 3);
            if (cnt > ND_ICON_MAX_COLORS) cnt = ND_ICON_MAX_COLORS;
            for (i = 0; i < cnt; ++i)
                memcpy(icon->palette[i], data + i * 3, 3);
            icon->colors = (unsigned char)cnt;
            has_cmap = cnt > 0;
        } else if (!memcmp(buf + pos, "BODY", 4)) {
            body_pos = pos + 8; body_len = chunk_len;
        }
        pos += 8 + chunk_len + (chunk_len & 1UL);
    }
    /* CMAP may legally contain fewer entries than the plane capacity.  The
     * missing indices are simply unmapped; this is common in compact
     * palette-generated decorative ILBMs. */
    if (!has_bmhd || !has_cmap || !body_pos) goto done;
    rowbytes = ((w + 15U) / 16U) * 2U;
    if (rowbytes * planes > sizeof(plane)) goto done;
    pos = 0;
    for (y = 0; y < h; ++y) {
        for (p = 0; p < planes; ++p) {
            if (!decode_row(buf + body_pos, body_len, &pos,
                            plane + p * rowbytes, rowbytes, compression)) goto done;
        }
        for (x = 0; x < w; ++x) {
            unsigned int color = 0;
            for (p = 0; p < planes; ++p)
                if (plane[p * rowbytes + x / 8U] & (0x80U >> (x & 7U)))
                    color |= 1U << p;
            icon->pixels[y * ND_ICON_MAX_DIM + x] = (unsigned char)color;
        }
    }
    icon->width = (unsigned short)w;
    icon->height = (unsigned short)h;
    success = 1;
done:
    if (buf) free(buf);
    fclose(fp);
    return success;
}
