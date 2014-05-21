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
 * @file zlog.h
 * @brief Logger header.
 * @author 招牌疯子 zp@buaa.us
 * @version 1.0
 * @date 2013-07-19
 */


#ifndef ZLOG_H
#define ZLOG_H

#include <pthread.h>
#include "zcommon.h"
#include "zspinlock.h"

#define MAX_LOGS        100
 
#define LOG_INVALID     -1
#define LOG_SYSTEM       0
#define LOG_USER         1
 
enum LOG_LEVEL{
    LOG_LEVEL_FATAL = 0,                        /* System is unusable */
    LOG_LEVEL_ALERT,                        /* Action must be taken immediately */
    LOG_LEVEL_CRIT,                         /* Critical conditions */
    LOG_LEVEL_ERROR,                        /* Error conditions */
    LOG_LEVEL_WARNING,                      /* Warning conditions */
    LOG_LEVEL_NOTICE,                       /* Normal, but significant */
    LOG_LEVEL_INFO,                         /* Information */
    LOG_LEVEL_DEBUG,                        /* DEBUG message */
};

void log_init();
int log_open(const char *path, const char* mode);
void log_printf0(int log_id, int level, const char *fmt, ...);
void log_flush(int log_id);
void log_close(int log_id);

#endif
