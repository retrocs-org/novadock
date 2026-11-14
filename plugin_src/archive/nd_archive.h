#ifndef ND_ARCHIVE_H
#define ND_ARCHIVE_H

#define ND_ARCHIVE_MAX_SOURCES 128
struct NDArchiveSource {char path[260];int drawer,include_info;};
/* Remove redundant selections; return 0 on unsafe paths or name collisions. */
unsigned int nd_archive_plan_sources(struct NDArchiveSource *sources,unsigned int count);
#define ND_ARCHIVE_NONE 0
#define ND_ARCHIVE_ZIP 1
#define ND_ARCHIVE_LHA 2
#define ND_ARCHIVE_RAR 3
#define ND_ARCHIVE_ARJ 4
#define ND_ARCHIVE_LZX 5
#define ND_ARCHIVE_TAR 6
#define ND_ARCHIVE_TGZ 7  /* .tar.gz and .tgz */
#define ND_ARCHIVE_GZ 8   /* single gzip-compressed file */
#define ND_ARCHIVE_LAST ND_ARCHIVE_GZ

/* Per-extraction status files: one file per helper process, so concurrent
 * drops never overwrite each other's progress. */
#define ND_ARCHIVE_STATUS_PATTERN "T:NovaDockArchive-#?.status"

int nd_archive_kind(const char *path);
int nd_archive_detect(const char *path,const unsigned char *header,unsigned int length);
int nd_archive_output_name(const char *name,int kind,char *output,unsigned int size);
int nd_archive_compress_command(int kind,const char *tool,const char *source,
                                const char *output,int include_info,char *command,unsigned int size);
int nd_archive_output_allowed(const char *source,int drawer,const char *output);
int nd_archive_extract_destination(const char *parent,int create_drawer,const char *name,
                                   char *output,unsigned int size);
int nd_archive_safe_path(const char *path);
/* Destination drawer beside the archive; both extensions of .tar.gz are
 * removed. */
int nd_archive_destination(const char *archive,char *destination,
                           unsigned int size);
/* For ND_ARCHIVE_GZ: the decompressed file name inside DESTINATION. */
int nd_archive_gz_output(const char *archive,const char *destination,
                         char *output,unsigned int size);
/* Build the command line.  The selected TOOL decides the syntax (for
 * example LZX versus unlzx, or ARJ versus UnARJ).  Tools that can only
 * extract into the current directory are run after a "CD" line, so the
 * result can contain two newline-separated shell lines. */
int nd_archive_command(int kind,const char *tool,const char *archive,
                       const char *destination,char *command,
                       unsigned int size);
int nd_archive_status_name(unsigned long owner,char *name,unsigned int size);
/* Human-readable list of the commands searched for KIND (for messages). */
const char *nd_archive_tool_names(int kind);

/* Incremental console raw-event parser; zero-initialise before reading. */
struct NDArchiveCloseInput {unsigned int state,field,part[3];};
int nd_archive_close_input(struct NDArchiveCloseInput *input,unsigned char byte);
#endif
