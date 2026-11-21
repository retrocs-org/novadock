#ifndef ND_QR_VIEW_H
#define ND_QR_VIEW_H
#include "nd_qr.h"
struct NDQRView;
struct NDQRView *nd_qr_view_open(const struct NDQR *qr);
const char *nd_qr_view_error(void);
void nd_qr_view_close(struct NDQRView *view);
void nd_qr_view_raise(struct NDQRView *view);
unsigned long nd_qr_view_signals(const struct NDQRView *view);
int nd_qr_view_event(struct NDQRView *view);
#endif
