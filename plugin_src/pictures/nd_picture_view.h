#ifndef ND_PICTURE_VIEW_H
#define ND_PICTURE_VIEW_H
#include "nd_pictures.h"
struct NDPictureView;
struct NDPictureView *nd_picture_view_open(const struct NDPictureImage *image,const char *name);
void nd_picture_view_set(struct NDPictureView *view,const struct NDPictureImage *image,const char *name);
void nd_picture_view_close(struct NDPictureView *view);
void nd_picture_view_raise(struct NDPictureView *view);
unsigned long nd_picture_view_signals(const struct NDPictureView *view);
int nd_picture_view_event(struct NDPictureView *view);
#endif
