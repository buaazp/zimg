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
 * @file zlog.c
 * @brief zimg log functions.
 * @author 招牌疯子 zp@buaa.us
 * @version 3.0.0
 * @date 2014-08-14
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <stdarg.h>
#include "zlog.h"

static int log_valid(int log_id);
void log_init(void);
int log_open(const char *path, const char* mode);
void log_handler(const char *msg);
void log_printf0(int log_id, int log_level, const char *fmt, ...);
void log_flush(int log_id);
void log_close(int log_id);

struct log_level_desc {
    enum LOG_LEVEL  level;
    char*           endesc;
    wchar_t*        cndesc;
};

static struct log_level_desc log_level_descs[] = {
    { LOG_LEVEL_FATAL,      "FATAL",        L"致命" },
    { LOG_LEVEL_ALERT,      "ALERT",        L"危急" },
    { LOG_LEVEL_CRIT,       "CRITICAL",     L"紧急" },
    { LOG_LEVEL_ERROR,      "ERROR",        L"错误" },
    { LOG_LEVEL_WARNING,    "WARNING",      L"警告" },
    { LOG_LEVEL_NOTICE,     "NOTICE",       L"注意" },
    { LOG_LEVEL_INFO,       "INFO",         L"消息" },
    { LOG_LEVEL_DEBUG,      "DEBUG",        L"调试" },
};

static FILE* log_files[MAX_LOGS + 1];
static spin_lock_t log_locks[MAX_LOGS + 1];

/**
 * @brief log_valid check log is valid
 *
 * @param log_id the log id
 *
 * @return 1 for OK and 0 for fail
 */
static int log_valid(int log_id) {
    if (log_id < 0 || log_id > MAX_LOGS)
        return 0;

    return 1;
}

/**
 * @brief log_init init log
 */
void log_init(void) {
    int i;

    for (i = 0; i < MAX_LOGS + 1; i++) {
        log_files[i] = NULL;
        spin_init(&log_locks[i], NULL);
    }
}

/**
 * @brief log_open open a log file
 *
 * @param path the path of log
 * @param mode the mode of open
 *
 * @return log open id for OK and -1 for fail
 */
int log_open(const char *path, const char* mode) {
    int i;

    for (i = LOG_USER; i < MAX_LOGS + 1; i++) {
        spin_lock(&log_locks[i]);

        if (log_files[i] == NULL) {
            log_files[i] = fopen(path, mode);

            if (log_files[i]) {
                spin_unlock(&log_locks[i]);
                return i;
            }
        }

        spin_unlock(&log_locks[i]);
    }

    return LOG_INVALID;
}

/* Log a fixed message without printf-alike capabilities, in a way that is
 * safe to call from a signal handler.
 *
 * We actually use this only for signals that are not fatal from the point
 * of view of Redis. Signals that are going to kill the server anyway and
 * where we need printf-alike features are served by redisLog(). */
void log_handler(const char *msg) {
    int fd;
    int log_to_stdout = settings.log_level == -1;

    fd = log_to_stdout ? STDOUT_FILENO :
         open(settings.log_name, O_APPEND | O_CREAT | O_WRONLY, 0644);
    if (fd == -1) return;
    if (write(fd, msg, strlen(msg)) == -1) goto err;
    if (write(fd, "\n", 1) == -1) goto err;
err:
    if (!log_to_stdout) close(fd);
}

/**
 * @brief log_printf0 print log to file function
 *
 * @param log_id log id
 * @param log_level log level
 * @param fmt format of string
 * @param ... other args
 */
void log_printf0(int log_id, int log_level, const char *fmt, ...) {
    FILE *fp;
    time_t t;
    char tmbuf[30];
    const char *p;
    va_list args;
    int level;

    if (!log_valid(log_id))
        fp = stdout;
    else {
        spin_lock(&log_locks[log_id]);
        if (!(fp = log_files[log_id])) {
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

    t = time(NULL);
    struct timeval tv;
    gettimeofday(&tv , NULL);
    memset(tmbuf, 0, sizeof(tmbuf));
    strftime(tmbuf, sizeof(tmbuf), "%Y/%m/%d %H:%M:%S", localtime(&t));
    fprintf (fp, "%s:%.6d ", tmbuf, (int)tv.tv_usec);

#ifdef DEBUG
    fprintf(fp, "Thread ID: %lu ", (unsigned long)pthread_self());
#endif

    fprintf(fp, "[%s] ", log_level_descs[level].endesc);

    va_start(args, fmt);
    vfprintf(fp, fmt, args);
    va_end(args);

    p = fmt + strlen(fmt) - 1;
    if (*p != '\n')
        fputc('\n', fp);

    if (log_valid(log_id)) {
        fflush(fp);
        spin_unlock(&log_locks[log_id]);
    }
}

/**
 * @brief log_flush flush log string to file
 *
 * @param log_id log id
 */
void log_flush(int log_id) {
    if (!log_valid(log_id))
        return;

    spin_lock(&log_locks[log_id]);

    if (log_files[log_id])
        fflush(log_files[log_id]);

    spin_unlock(&log_locks[log_id]);
}

/**
 * @brief log_close close the log
 *
 * @param log_id the log id
 */
void log_close(int log_id) {
    if (!log_valid(log_id))
        return;

    spin_lock(&log_locks[log_id]);

    if (log_files[log_id]) {
        fclose(log_files[log_id]);
        log_files[log_id] = NULL;
    }

    spin_unlock(&log_locks[log_id]);
}
