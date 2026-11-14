#ifndef ND_ARCHIVE_DIALOG_H
#define ND_ARCHIVE_DIALOG_H
/* 1 accepted, 0 cancelled, -1 UI unavailable. MASK: ZIP=1, LHA=2. */
struct NDArchiveSource;
int nd_archive_compression_request(const struct NDArchiveSource *sources,unsigned int count,
                                   int require_name,int mask,int *kind,
                                   char *output,unsigned int size,int *create_icon);
int nd_archive_extraction_request(const char *archive,char *destination,
                                  unsigned int size,int *create_drawer,int *create_icon);
#endif
