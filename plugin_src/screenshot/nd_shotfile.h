#ifndef ND_SHOTFILE_H
#define ND_SHOTFILE_H
/* Portable IFF ILBM writer for screenshots: up to 8 bitplanes, ByteRun1
 * compressed, with CMAP and CAMG (display mode).  Rows are given as one
 * byte per pixel (pen numbers), as graphics.library ReadPixelLine8 returns. */
#include <stdio.h>
#define ND_SHOT_MAX_WIDTH 4096
struct NDShotWriter {
    FILE *file;
    long form_at,body_at;
    unsigned int width,height,depth,row;
    unsigned int plane_bytes;      /* bytes per bitplane row (even) */
    unsigned char *planar;         /* one plane row */
    unsigned char *packed;         /* ByteRun1 output for one plane row */
    int failed;
};
/* ByteRun1 (PackBits) of SRC[0..N); DST needs N+N/128+2 bytes.  Returns
 * the packed length. */
unsigned int nd_byterun1(const unsigned char *src,unsigned int n,
                         unsigned char *dst);
/* PALETTE holds COLORS RGB triplets (0..255).  Returns 1 on success. */
int nd_shot_begin(struct NDShotWriter *w,const char *path,unsigned int width,
                  unsigned int height,unsigned int depth,
                  const unsigned char *palette,unsigned int colors,
                  unsigned long mode_id);
int nd_shot_row(struct NDShotWriter *w,const unsigned char *pens);
/* Finishes the file (sizes, padding).  Returns 1 if everything was
 * written; on failure the partial file is removed. */
int nd_shot_end(struct NDShotWriter *w,const char *path);
/* "Screenshot-YYYYMMDD-HHMMSS.iff" in DRAWER for an AmigaDOS DateStamp
 * (days since 1 Jan 1978, minutes, ticks); SERIAL>0 adds "-n" so a second
 * shot within the same second gets its own name.  Returns 0 if too long. */
int nd_shot_name(char *out,unsigned int size,const char *drawer,long days,
                 long minute,long tick,int serial);
#endif
