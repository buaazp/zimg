#ifndef ZSCALE_H
#define ZSCALE_H


#include "zcommon.h"

static int proportion(struct image *im, uint32_t arg_cols, uint32_t arg_rows);
int convert(struct image *im, zimg_req_t *req);

#endif
