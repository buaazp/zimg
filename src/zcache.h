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
 * @file zcache.h
 * @brief header of memcached functions.
 * @author 招牌疯子 zp@buaa.us
 * @version 1.0
 * @date 2013-07-23
 */

#ifndef ZCACHE_H
#define ZCACHE_H

#include "zcommon.h"


int exist_cache(const char *key);
int find_cache(const char *key, char *value);
int set_cache(const char *key, const char *value);
int find_cache_bin(const char *key, char **value_ptr, size_t *len);
int set_cache_bin(const char *key, const char *value, const size_t len);
int del_cache(const char *key);

#endif

