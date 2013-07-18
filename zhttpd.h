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

#include "zcommon.h"

void dump_request_cb(struct evhttp_request *req, void *arg);
void post_request_cb(struct evhttp_request *req, void *arg);
void send_document_cb(struct evhttp_request *req, void *arg);


#endif
