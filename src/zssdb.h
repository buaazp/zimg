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
 * @file zssdb.h
 * @brief Get and save image for ssdb/redis backend functions' header.
 * @author 招牌疯子 zp@buaa.us
 * @version 2.0.0
 * @date 2014-04-16
 */


#ifndef ZSSDB_H
#define ZSSDB_H


#include "zcommon.h"

int get_img_mode_ssdb(zimg_req_t *req, char **buff_ptr, size_t *img_size);
int save_img_ssdb(const char *cache_key, const char *buff, const size_t len);
int get_img_ssdb(const char *cache_key, char **buff, size_t *len);


#endif
