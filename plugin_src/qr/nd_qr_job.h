#ifndef ND_QR_JOB_H
#define ND_QR_JOB_H
#include "nd_qr.h"
struct QRJob {char text[ND_QR_TEXT_MAX+1],path[240];struct NDQR qr;};
#endif
