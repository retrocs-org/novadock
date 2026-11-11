#ifndef ND_ILBM_H
#define ND_ILBM_H
/* Portable minimal IFF ILBM brush reader. 32/64px classic icons and dock
 * skins up to the default 82-pixel window height.
 * Supports transparent palette index; ByteRun1 and raw BODY data.
 */
#define ND_ICON_MAX_DIM 96
#define ND_ICON_MAX_COLORS 16
struct NDILBM {
    unsigned short width;
    unsigned short height;
    unsigned char pixels[ND_ICON_MAX_DIM * ND_ICON_MAX_DIM];
    unsigned char palette[ND_ICON_MAX_COLORS][3];
    unsigned char colors;
    short transparent; /* -1 for fully opaque, else transparent index */
};
int nd_ilbm_load(const char *filename, struct NDILBM *icon);
#endif
