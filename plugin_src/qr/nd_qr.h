#ifndef ND_QR_H
#define ND_QR_H
#include "vendor/qrcodegen.h"
#define ND_QR_TEXT_MAX 512
#define ND_QR_VERSION 18
#define ND_QR_BYTES qrcodegen_BUFFER_LEN_FOR_VERSION(ND_QR_VERSION)
#define ND_QR_ILBM_MAX 6000
struct NDQR {unsigned char code[ND_QR_BYTES];unsigned int size;};
int nd_qr_generate(const char *text,struct NDQR *out);
unsigned int nd_qr_ilbm(const struct NDQR *qr,unsigned char *out,unsigned int cap);
int nd_qr_pixel(const struct NDQR *qr,int x,int y); /* includes four-module quiet zone */
#endif
