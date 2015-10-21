/*
 *   zimg - high performance image storage and processing system.
 *       http://zimg.buaa.us
 *
 *   Copyright (c) 2013-2014, Peter Zhao <zp@buaa.us>.
 *   All rights reserved.
 *
 *   Use and distribution licensed under the BSD license.
 *   See the LICENSE file for full text.
 *
 */

/**
 * @file zscale.h
 * @brief scale image functions by graphicsmagick header.
 * @author 招牌疯子 zp@buaa.us
 * @version 3.2.0
 * @date 2015-10-24
 */

#ifndef ZSCALE_H
#define ZSCALE_H

#include "zcommon.h"
#include <wand/magick_wand.h>

int convert(MagickWand *im, zimg_req_t *req);

#endif
