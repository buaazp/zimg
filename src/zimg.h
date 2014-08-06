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


int save_img(thr_arg_t *thr_arg, const char *buff, const int len, char *md5);
int new_img(const char *buff, const size_t len, const char *save_name);
int get_img(zimg_req_t *req, evhtp_request_t *request);
int get_img2(zimg_req_t *req, evhtp_request_t *request);
int admin_img(evhtp_request_t *req, const char *md5, int t);
int info_img(char *md5, evhtp_request_t *request);


#endif
