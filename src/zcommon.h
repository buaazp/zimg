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
 * @file zcommon.h
 * @brief common data structs header.
 * @author 招牌疯子 zp@buaa.us
 * @version 3.0.0
 * @date 2014-08-14
 */

#ifndef ZCOMMON_H
#define ZCOMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <libmemcached/memcached.h>
#include <hiredis/hiredis.h>
#include <lualib.h>
#include "libevhtp/evhtp.h"
#include "zaccess.h"
#include "zhttpd.h"
#include "multipart-parser-c/multipart_parser.h"

#ifndef PROJECT_VERSION
#define PROJECT_VERSION "3.1.1"
#endif

#define MAX_LINE            1024
#define CACHE_MAX_SIZE      1048576 //1024*1024
#define RETRY_TIME_WAIT     1000
#define CACHE_KEY_SIZE      128
#define PATH_MAX_SIZE       512

typedef struct thr_arg_s {
    evthr_t *thread;
    memcached_st *cache_conn;
    memcached_st *beansdb_conn;
    redisContext *ssdb_conn;
    lua_State* L;
} thr_arg_t;

typedef struct zimg_req_s {
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
} zimg_req_t;

struct setting{
    lua_State *L;
    int is_daemon;
    char ip[128];
    int port;
    int num_threads;
    int backlog;
    int max_keepalives;
    int retry;
    char version[128];
    char server_name[128];
    zimg_headers_conf_t *headers;
    int etag;
    zimg_access_conf_t *up_access;
    zimg_access_conf_t *down_access;
    zimg_access_conf_t *admin_access;
    int cache_on;
    char cache_ip[128];
    int cache_port;
    int log_level;
    char log_name[512];
    char root_path[512];
    char admin_path[512];
    int disable_args;
    int disable_type;
    int disable_zoom_up;
    int script_on;
    char script_name[512];
    char format[16];
    int quality;
    int mode;
    int save_new;
    int max_size;
    char img_path[512];
    char beansdb_ip[128];
    int beansdb_port;
    char ssdb_ip[128];
    int ssdb_port;
    multipart_parser_settings *mp_set;
	int (*get_img)(zimg_req_t *, evhtp_request_t *);
    int (*info_img)(evhtp_request_t *, thr_arg_t *, char *);
    int (*admin_img)(evhtp_request_t *, thr_arg_t *, char *, int);
    char pemfile[512]; /* add ssl */
    char privfile[512];
    char cafile[512];
} settings;

#define LOG_FATAL       0           /* System is unusable */
#define LOG_ALERT       1           /* Action must be taken immediately */
#define LOG_CRIT        2           /* Critical conditions */
#define LOG_ERROR       3           /* Error conditions */
#define LOG_WARNING     4           /* Warning conditions */
#define LOG_NOTICE      5           /* Normal, but significant */
#define LOG_INFO        6           /* Information */
#define LOG_DEBUG       7           /* DEBUG message */

#ifdef DEBUG
  #define LOG_PRINT(level, fmt, ...)            \
    do { \
        int log_id = log_open(settings.log_name, "a"); \
        log_printf0(log_id, level, "%s:%d %s() "fmt,   \
        __FILE__, __LINE__, __FUNCTION__, \
        ##__VA_ARGS__); \
        log_close(log_id); \
    }while(0)
#else
  #define LOG_PRINT(level, fmt, ...)            \
    do { \
        if (level <= settings.log_level) { \
            int log_id = log_open(settings.log_name, "a"); \
            log_printf0(log_id, level, fmt, ##__VA_ARGS__) ; \
            log_close(log_id); \
        } \
    }while(0)
#endif

#endif
