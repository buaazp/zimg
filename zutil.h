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
 * @file zutil.h
 * @brief Util header.
 * @author 招牌疯子 zp@buaa.us
 * @version 1.0
 * @date 2013-07-19
 */


#ifndef ZUTIL_H
#define ZUTIL_H

#include "zcommon.h"

pid_t gettid();
int kmp(const char *matcher, int mlen, const char *pattern, int plen);
int get_type(const char *filename, char *type);
int is_img(const char *filename);
int is_dir(const char *path);
int mk_dir(const char *path);
int mk_dirs(const char *dir);
int is_md5(char *s);
int str_hash(const char *str);


#endif
