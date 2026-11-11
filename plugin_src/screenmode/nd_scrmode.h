#ifndef ND_SCRMODE_H
#define ND_SCRMODE_H
/* Portable reader for Workbench ScreenMode preferences files (presets saved
 * with ScreenMode's "Save As..."): an IFF FORM PREF with a SCRM chunk. */
#define ND_SCRMODE_DEFAULT 0xFFFFU /* width/height: the mode's default */
struct NDScreenModePreset {
    unsigned long display_id;
    unsigned int width,height,depth,control;
};
/* Returns 1 when BUFFER is a ScreenMode preferences file. */
int nd_scrmode_parse(const unsigned char *buffer,unsigned long length,
                     struct NDScreenModePreset *preset);
/* "800x600x8"; a default dimension is shown as "auto".  Returns 0 if
 * SIZE is too small. */
int nd_scrmode_text(unsigned int width,unsigned int height,unsigned int depth,
                    char *out,unsigned int size);
#endif
