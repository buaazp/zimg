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
 * @file zimg.h
 * @brief Convert, get and save image functions header.
 * @author 招牌疯子 zp@buaa.us
 * @version 3.2.0
 * @date 2015-10-24
 */

#ifndef ZIMG_H
#define ZIMG_H

#include "zcommon.h"

int save_img(thr_arg_t *thr_arg, const char *buff, const int len, char *md5);
int new_img(const char *buff, const size_t len, const char *save_name);
int get_img(zimg_req_t *req, evhtp_request_t *request);
int admin_img(evhtp_request_t *req, thr_arg_t *thr_arg, char *md5, int t);
int info_img(evhtp_request_t *request, thr_arg_t *thr_arg, char *md5);

#endif
