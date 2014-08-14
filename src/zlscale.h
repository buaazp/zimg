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
 * @file zlscale.h
 * @brief processing image with lua script header.
 * @author 招牌疯子 zp@buaa.us
 * @version 3.0.0
 * @date 2014-08-14
 */

#ifndef ZLSCALE_H
#define ZLSCALE_H

#include "webimg/webimg2.h"

typedef struct lua_arg_s {
    struct image *img;
    char *trans_type;
    int lua_ret;
} lua_arg;

int lua_convert(struct image *im, zimg_req_t *req);

#endif
