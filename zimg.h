/*   
 *   zimg - high performance image storage and processing system.
 *       http://zimg.buaa.us 
 *   
 *   Copyright (c) 2013, Peter Zhao <zp@buaa.us>.
 *   All rights reserved.
 *   
 *   Use and distribution licensed under the BSD license.
 *   See the LICENSE file for full text.
 * 
 */

/**
 * @file zimg.h
 * @brief Processing images header.
 * @author 招牌疯子 zp@buaa.us
 * @version 1.0
 * @date 2013-07-19
 */


#ifndef ZIMG_H
#define ZIMG_H


#include "zcommon.h"

typedef struct zimg_req_s {
    char *md5;
    int width;
    int height;
    bool proportion;
    bool gray;
	char *rspPath;
} zimg_req_t;

int save_img(const char *buff, const int len, char *md5);
int new_img(const char *buff, const size_t len, const char *saveName);
int get_img(zimg_req_t *req, char **buff_ptr, size_t *img_size);


#endif
