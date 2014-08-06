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

char *strnchr(const char *p, char c, size_t n);
char * strnstr(const char *s, const char *find, size_t slen);
size_t str_lcat(char *dst, const char *src, size_t size);
size_t str_lcpy(char *dst, const char *src, size_t size);
int bind_check(int port);
pid_t gettid();
int get_cpu_cores();
int get_type(const char *filename, char *type);
int is_file(const char *filename);
int is_img(const char *filename);
int is_dir(const char *path);
int is_special_dir(const char *path);
void get_file_path(const char *path, const char *file_name, char *file_path);
int mk_dir(const char *path);
int mk_dirs(const char *dir);
int mk_dirf(const char *filename);
int delete_file(const char *path);
int is_md5(char *s);
int str_hash(const char *str);
int gen_key(char *key, char *md5, ...);

#endif
