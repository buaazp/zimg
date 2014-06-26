#ifndef ZSCALE_H
#define ZSCALE_H


#include "zcommon.h"

#define WAP_QUALITY				70
#define THUMB_QUALITY			85
#define QUALITY_90				90
#define QUALITY_95				95

static int proportion(struct image *im, int p_type, uint32_t cols, uint32_t rows);
int convert(struct image *im, zimg_req_t *req);

#endif
