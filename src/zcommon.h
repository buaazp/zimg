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
 * @file zcommon.h
 * @brief Common header.
 * @author 招牌疯子 zp@buaa.us
 * @version 1.0
 * @date 2013-07-19
 */


#ifndef ZCOMMON_H
#define ZCOMMON_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <libmemcached/memcached.h>
#include <hiredis/hiredis.h>
#include <stdbool.h>
#include "libevhtp/evhtp.h"
#include "zaccess.h"

#ifndef ZIMG_VERSION
#define ZIMG_VERSION "2.0.0"
#endif

#define MAX_LINE 1024 
#define CACHE_MAX_SIZE 1024*1024
#define RETRY_TIME_WAIT 1000
/* Number of worker threads.  Should match number of CPU cores reported in /proc/cpuinfo. */
#define NUM_THREADS 4

struct setting{
    int is_daemon;
    char root_path[512];
    char img_path[512];
    char log_name[512];
    char version[128];
    char server_name[128];
    int port;
    int backlog;
    int num_threads;
    bool log;
    bool cache_on;
    char cache_ip[128];
    int cache_port;
    uint64_t max_keepalives;
    int mode;
    char beansdb_ip[128];
    int beansdb_port;
    char ssdb_ip[128];
    int ssdb_port;
    int retry;
    zimg_access_conf_t *up_access;
    zimg_access_conf_t *down_access;
} settings;

typedef struct thr_arg_s {
    evthr_t *thread;
    memcached_st *cache_conn;
    memcached_st *beansdb_conn;
    redisContext *ssdb_conn;
} thr_arg_t;

typedef struct zimg_req_s {
    char *md5;
    int width;
    int height;
    bool proportion;
    bool gray;
    char *rsp_path;
    thr_arg_t *thr_arg;
} zimg_req_t;

char *_init_path;

struct timespec retry_sleep;

#define LOG_FATAL 0                        /* System is unusable */
#define LOG_ALERT 1                        /* Action must be taken immediately */
#define LOG_CRIT 2                       /* Critical conditions */
#define LOG_ERROR 3                        /* Error conditions */
#define LOG_WARNING 4                      /* Warning conditions */
#define LOG_NOTICE 5                      /* Normal, but significant */
#define LOG_INFO 6                      /* Information */
#define LOG_DEBUG 7                       /* DEBUG message */


#ifdef DEBUG 
  #define LOG_PRINT(level, fmt, ...)            \
    do { \
        int log_id = log_open(settings.log_name, "a"); \
        log_printf0(log_id, level, "%s:%d %s() "fmt,   \
        __FILE__, __LINE__, __FUNCTION__, \
        ##__VA_ARGS__); \
        log_close(log_id); \
    }while(0) 

  #define ThrowWandException(wand) \
  { \
      char *description; \
      ExceptionType severity; \
      description=MagickGetException(wand,&severity); \
      LOG_PRINT(LOG_ERROR, "%s %s %lu %s",GetMagickModule(),description); \
      description=(char *) MagickRelinquishMemory(description); \
  }
#else
  #define LOG_PRINT(level, fmt, ...)            \
    do { \
        int log_id = log_open(settings.log_name, "a"); \
        log_printf0(log_id, level, fmt, ##__VA_ARGS__) ; \
        log_close(log_id); \
    }while(0) 
  #define ThrowWandException(wand) 0
#endif
 
#endif
