/*
 *   zimg - high performance image storage and processing system.
 *       http://zimg.buaa.us
 *
 *   Copyright (c) 2013-2015, Peter Zhao <zp@buaa.us>.
 *   All rights reserved.
 *
 *   Use and distribution licensed under the BSD license.
 *   See the LICENSE file for full text.
 *
 */

/**
 * @file zlscale.h
 * @brief processing image with lua script header.
 * @author 招牌疯子 zp@buaa.us
 * @version 3.2.0
 * @date 2015-10-24
 */

#ifndef ZLSCALE_H
#define ZLSCALE_H

#include <wand/magick_wand.h>

typedef struct lua_arg_s {
    MagickWand *img;
    char *trans_type;
    int lua_ret;
} lua_arg;

int lua_convert(MagickWand *im, zimg_req_t *req);

#endif
