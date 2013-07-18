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
 * @file zlog.c
 * @brief Logger functions.
 * @author 招牌疯子 zp@buaa.us
 * @version 1.0
 * @date 2013-07-19
 */

#include "zlog.h"

extern struct setting settings;

static int log_valid(int log_id);
void log_init();
int log_open(const char *path, const char* mode);
void log_printf0(int log_id, int log_level, const char *fmt, ...);
void log_flush(int log_id);
void log_close(int log_id);

/* 日志记录 */
struct log_level_desc{
    enum LOG_LEVEL  level;
    char*           endesc;
    wchar_t*        cndesc;
};
 
static struct log_level_desc log_level_descs[] = {
    { LOG_LEVEL_FATAL,  "FATAL",        L"致命" },
    { LOG_LEVEL_ALERT,  "ALERT",        L"危急" },
    { LOG_LEVEL_CRIT,   "CRITICAL",     L"紧急" },
    { LOG_LEVEL_ERROR,  "ERROR",        L"错误" },
    { LOG_LEVEL_WARNING,    "WARNING",      L"警告" },
    { LOG_LEVEL_NOTICE, "NOTICE",       L"注意" },
    { LOG_LEVEL_INFO,   "INFO",         L"消息" },
    { LOG_LEVEL_DEBUG,  "DEBUG",        L"调试" },
};
 
static FILE* log_files[MAX_LOGS+1];
static spin_lock_t log_locks[MAX_LOGS+1];
 
static int log_valid(int log_id)
{
    if (log_id < 0 || log_id > MAX_LOGS)
        return 0;
 
    return 1;
}
 
/* 初始化日志模块 */
void log_init()
{
    int i;
 
    for (i = 0; i < MAX_LOGS+1; i++)
    {
        log_files[i] = NULL;
        spin_init(&log_locks[i], NULL);
    }
}
 
/* 打开用户日志文件 */
int log_open(const char *path, const char* mode)
{
    int i;
 
    for (i = LOG_USER; i < MAX_LOGS+1; i++)
    {
        spin_lock(&log_locks[i]);
 
        if (log_files[i] == NULL)
        {
            log_files[i] = fopen(path, mode);
 
            if (log_files[i])
            {
                spin_unlock(&log_locks[i]);
                return i;
            }
        }
 
        spin_unlock(&log_locks[i]);
    }
 
    return LOG_INVALID;
}
 
/* 写入日志文件 */
void log_printf0(int log_id, int log_level, const char *fmt, ...)
{
    if(settings.log == false)
        return;

    FILE *fp;
    time_t t;
    char tmbuf[30];
    const char *p;
    va_list args;
    int level;

#ifndef _DEBUG
    if (log_level == LOG_LEVEL_DEBUG)
        return;
#endif
 

    if (!log_valid(log_id))
        fp = stdout;
    else
    {
        spin_lock(&log_locks[log_id]);
        if (!(fp = log_files[log_id]))
        {
            spin_unlock(&log_locks[log_id]);
            return;
        }
    }
 
    if (log_level > LOG_LEVEL_DEBUG)
        level = LOG_LEVEL_DEBUG;
    else if (log_level < LOG_LEVEL_FATAL)
        level = LOG_LEVEL_FATAL;
    else
        level = log_level;
 
    //时间信息
    t = time(NULL);
    struct timeval tv;
    gettimeofday(&tv , NULL);
    memset(tmbuf, 0, sizeof(tmbuf));
    strftime(tmbuf, sizeof(tmbuf), "%Y/%m/%d %H:%M:%S", localtime(&t));
    fprintf (fp, "%s:%.6d ", tmbuf, tv.tv_usec);

    //线程信息
    fprintf(fp, "Thread ID: %lu ", (unsigned long)pthread_self());

    //等级信息
    fprintf(fp, "[%s] ", log_level_descs[level].endesc);
 
    //正文信息
    va_start(args, fmt);
    vfprintf(fp, fmt, args);
    va_end(args);
 
    //换行符
    p = fmt + strlen(fmt) - 1;
    if (*p != '\n')
        fputc('\n', fp);
 

    if (log_valid(log_id))
    {
        //缓冲区数据写入文件流
        fflush(fp);

        spin_unlock(&log_locks[log_id]);
    }
}
 
/* 将缓冲区数据写入文件 */
void log_flush(int log_id)
{
    if (!log_valid(log_id))
        return;
 
    spin_lock(&log_locks[log_id]);
 
    if (log_files[log_id])
        fflush(log_files[log_id]);
 
    spin_unlock(&log_locks[log_id]);
}
 
/* 关闭日志文件 */
void log_close(int log_id)
{
    if (!log_valid(log_id))
        return;
 
    spin_lock(&log_locks[log_id]);
 
    if (log_files[log_id])
    {
        fclose(log_files[log_id]);
        log_files[log_id] = NULL;
    }
 
    spin_unlock(&log_locks[log_id]);
}

