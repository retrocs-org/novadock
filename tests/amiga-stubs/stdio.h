#ifndef _STDIO_H
#define _STDIO_H
#include <stddef.h>
#include <stdarg.h>
typedef struct _FILE FILE;
extern FILE *stdin,*stdout,*stderr;
#define EOF (-1)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
FILE *fopen(const char*,const char*); int fclose(FILE*); int fprintf(FILE*,const char*,...);
int printf(const char*,...); int sprintf(char*,const char*,...); int vsprintf(char*,const char*,va_list);
char *fgets(char*,int,FILE*); int fputs(const char*,FILE*); int fputc(int,FILE*); int fgetc(FILE*);
size_t fread(void*,size_t,size_t,FILE*); size_t fwrite(const void*,size_t,size_t,FILE*);
int remove(const char*); int rename(const char*,const char*); int fflush(FILE*); int ferror(FILE*); int feof(FILE*);
int fseek(FILE*,long,int); long ftell(FILE*); int sscanf(const char*,const char*,...); int puts(const char*); int putchar(int);
int getc(FILE*); int ungetc(int,FILE*); void rewind(FILE*);
#endif
