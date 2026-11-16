/* Adapted from Beam68. Copyright (c) 2026 Beam68 contributors.
 * MIT licence: see Aminet-parser-LICENSE.txt in the package. */
#ifndef NOVADOCK_AMINET_H
#define NOVADOCK_AMINET_H
#include <stdio.h>
#define ND_AMINET_PATH 97
#define ND_AMINET_QUERY 81
#define ND_AMINET_ROWS 50
struct NDAminetPackage { char path[ND_AMINET_PATH]; char title[121]; };
struct NDAminetResults { struct NDAminetPackage rows[ND_AMINET_ROWS]; int count,seen; };
struct NDAminetDetails { char summary[121],version[81],author[121],architecture[121],requires[121]; };
int nd_aminet_archive_header(const char *package,const unsigned char *data,unsigned int size);
int nd_aminet_path_valid(const char *path);
int nd_aminet_encode_query(const char *query,char *out,unsigned int cap);
int nd_aminet_readme_path(const char *package,char *out,unsigned int cap);
int nd_aminet_search(FILE *file,int skip,struct NDAminetResults *results);
int nd_aminet_details(FILE *file,struct NDAminetDetails *details);
void nd_aminet_plain(const char *text,char *out,unsigned int cap);
/* Validate a complete HTTP header. -1 means no content length. */
int nd_aminet_http_header(const char *header,unsigned long limit,long *length);
/* Overflow-safe percentage; -1 when total is unknown. */
int nd_aminet_percent(unsigned long bytes,unsigned long total);
/* Native browser helpers: bounded category and readme presentation. */
int nd_aminet_category_valid(const char *path);
int nd_aminet_categories(FILE *file,const char *parent,struct NDAminetResults *result);

#endif
