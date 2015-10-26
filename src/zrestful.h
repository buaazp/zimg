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
 * @file zrestful.h
 * @brief http restful handler header.
 * @author 招牌疯子 zp@buaa.us
 * @version 3.2.0
 * @date 2015-10-24
 */

#ifndef ZRESTFUL_H
#define ZRESTFUL_H

#include "libevhtp/evhtp.h"
#include "zcommon.h"

typedef enum {
    api_ok = 0,
    api_err_key_illegal = 1,
    api_err_pixel_over = 2,
    api_err_access_forbidden = 3,
    api_err_method_forbidden = 4,
    api_err_check_access = 5,
    api_err_not_exsit = 6,
    api_err_malloc = 7,
    api_err_open_file = 8,
    api_err_read_file = 9,
    api_err_imagick = 10,
} api_error_code;

typedef enum {
    GET = 0,
    HEAD = 1,
    POST = 2,
    PUT = 3,
    DELETE = 4,
    MKCOL = 5,
    COPY = 6,
    MOVE = 7,
    OPTIONS = 8,
    PROPFIND = 9,
    PROPATCH = 10,
    LOCK = 11,
    UNLOCK = 12,
    TRACE = 13,
    CONNECT = 14,
    PATCH = 15,
    UNKNOWN = 16,
} http_method;

typedef struct {
    size_t len;
    char *buff;
} zbuf_t;

typedef struct {
    evhtp_request_t *req;
    char *ip;
    char *md5;
    char *type;
    int width;
    int height;
    int proportion;
    int gray;
    int x;
    int y;
    int rotate;
    int quality;
    char *fmt;
    int sv;
    thr_arg_t *thr_arg;
    zbuf_t *buff_in;
    zbuf_t *buff_out;
} zreq_t;

void get_image_cb(evhtp_request_t *req, void *arg);

#endif
