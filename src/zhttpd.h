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
 * @file zhttpd.h
 * @brief Parse http protocol header.
 * @author 招牌疯子 zp@buaa.us
 * @version 1.0
 * @date 2013-07-19
 */


#ifndef ZHTTPD_H
#define ZHTTPD_H

#include "libevhtp/evhtp.h"
#include "multipart_parser.h"

typedef struct zimg_headers_s zimg_headers_t;

typedef struct {
    char key[128];
    char value[512];
} zimg_header_t;

struct zimg_headers_s {
    zimg_header_t *value;
    zimg_headers_t *next;
};

typedef struct {
    uint n;
    zimg_headers_t *headers;
} zimg_headers_conf_t;

int on_header_field(multipart_parser* p, const char *at, size_t length);
int on_header_value(multipart_parser* p, const char *at, size_t length);
int on_chunk_data(multipart_parser* p, const char *at, size_t length);
int zimg_etag_set(evhtp_request_t *request, char *buff, size_t len);
zimg_headers_conf_t * conf_get_headers(const char *hdr_str);
void free_headers_conf(zimg_headers_conf_t *hcf);
void dump_request_cb(evhtp_request_t *req, void *arg);
void echo_cb(evhtp_request_t *req, void *arg);
void post_request_cb(evhtp_request_t *req, void *arg);
void send_document_cb(evhtp_request_t *req, void *arg);
void admin_request_cb(evhtp_request_t *req, void *arg);


#endif
