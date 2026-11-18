#ifndef ND_SHELF_H
#define ND_SHELF_H
#include <stdio.h>
#define ND_SHELF_MAX 64
#define ND_SHELF_PATH 260
struct NDShelf {char paths[ND_SHELF_MAX][ND_SHELF_PATH];unsigned int count;};
int nd_shelf_add(struct NDShelf *s,const char *path);
int nd_shelf_remove(struct NDShelf *s,unsigned int index);
int nd_shelf_move(struct NDShelf *s,unsigned int index,int direction);
int nd_shelf_read(FILE *f,struct NDShelf *s);
int nd_shelf_write(FILE *f,const struct NDShelf *s);
int nd_shelf_save(const char *path,const struct NDShelf *s);
int nd_shelf_id(const char *id);
#endif
