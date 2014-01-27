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
#include <libmemcached/memcached.h>
#include <stdbool.h>

#define _DEBUG 1

#define MAX_LINE 1024 
#define CACHE_MAX_SIZE 1024*1024
/* Number of worker threads.  Should match number of CPU cores reported in /proc/cpuinfo. */
#define NUM_THREADS 4

struct setting{
    int daemon;
    char root_path[512];
    char img_path[512];
    char log_name[512];
    int port;
    int backlog;
    int num_threads;
    bool log;
    char cache_ip[128];
    int cache_port;
    bool cache_on;
    uint64_t max_keepalives;
} settings;


char *_init_path;

#define LOG_FATAL 0                        /* System is unusable */
#define LOG_ALERT 1                        /* Action must be taken immediately */
#define LOG_CRIT 2                       /* Critical conditions */
#define LOG_ERROR 3                        /* Error conditions */
#define LOG_WARNING 4                      /* Warning conditions */
#define LOG_NOTICE 5                      /* Normal, but significant */
#define LOG_INFO 6                      /* Information */
#define LOG_DEBUG 7                       /* DEBUG message */


#ifdef _DEBUG 
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
        int log_id = log_open(settings.log_name, "a"); \
        log_printf0(log_id, level, fmt, ##__VA_ARGS__) ; \
        log_close(log_id); \
    }while(0) 
#endif
 

#define ThrowWandException(wand) \
{ \
    char *description; \
    ExceptionType severity; \
    description=MagickGetException(wand,&severity); \
    LOG_PRINT(LOG_ERROR, "%s %s %lu %s",GetMagickModule(),description); \
    description=(char *) MagickRelinquishMemory(description); \
}

#endif
