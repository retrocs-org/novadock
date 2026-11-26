#ifndef ND_ASYNC_IO_H
#define ND_ASYNC_IO_H
#include "nd_config.h"
#include "nd_ilbm.h"
#include "nd_folder.h"
#define ND_IO_ICON 2U
#define ND_IO_LAUNCH 3U
#define ND_IO_FOLDER 4U
#define ND_IO_FOLDER_ICON 5U
struct NDFolderBatch {char path[ND_COMMAND_LEN];unsigned long error;unsigned int count,more;struct NDFolderEntry entries[ND_FOLDER_MAX];};
#define ND_PICTURE_SIDE 96
struct NDIconPicture {unsigned short width,height,colors;unsigned char rgb[256][3],pixels[96*96],opaque[96*96];};
struct NDIOJob {
 char path[512],arguments[ND_COMMAND_LEN],directory[ND_COMMAND_LEN];
 unsigned int fallback,picture,iff,preferences,mode;unsigned long error;
 struct DiskObject *icon;struct NDILBM *brush;struct NDIconPicture *decoded;
};
void nd_io_dispose(void *data,unsigned int bytes);
int nd_background_worker(const char *parent);
#endif
