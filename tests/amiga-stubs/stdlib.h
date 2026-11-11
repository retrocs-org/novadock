#ifndef _STDLIB_H
#define _STDLIB_H
#include <stddef.h>
void *malloc(size_t); void free(void*); void *calloc(size_t,size_t); void *realloc(void*,size_t);
int atoi(const char*); long atol(const char*); long strtol(const char*,char**,int); unsigned long strtoul(const char*,char**,int);
void exit(int); int abs(int); long labs(long); void qsort(void*,size_t,size_t,int(*)(const void*,const void*)); char *getenv(const char*);
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#endif
