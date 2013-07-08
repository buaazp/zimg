#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <signal.h>
/*
#include <event.h>
#include <evhttp.h>
*/
#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>
#ifdef _EVENT_HAVE_NETINET_IN_H
#include <netinet/in.h>
#ifdef _XOPEN_SOURCE_EXTENDED
#include <arpa/inet.h>
#endif
#endif
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include <wand/MagickWand.h>
#include <libmemcached/memcached.h>
#include <stdbool.h>

#define _DEBUG 1

#define MAX_LINE 1024 

char _root_path[512];
char _img_path[512];
int _port;
int _log_id;

memcached_st *_memc;

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
        log_printf0(_log_id, level, "%s:%d %s() "fmt,   \
        __FILE__, __LINE__, __FUNCTION__, \
        ##__VA_ARGS__)
#else
  #define LOG_PRINT(level, fmt, ...)            \
        log_printf0(_log_id, level, fmt, ##__VA_ARGS__)
#endif
 


//#ifdef _DEBUG
//  #define DEBUG_PRINT(fmt, args...) \
//    do{ \
//        time_t t; \
//        struct tm *p; \
//        struct timeval tv; \
//        gettimeofday (&tv , NULL); \
//        time(&t); \
//        p = localtime(&t); \
//        fprintf(stdout, "\033[40;32;m%.4d/%.2d/%.2d %.2d:%.2d:%.2d:%.6d [DEBUG] %s:%d %s() "fmt"\n\033[5m", \
//                (1900+p->tm_year), (1+p->tm_mon),  p->tm_mday, \
//                p->tm_hour, p->tm_min, p->tm_sec, tv.tv_usec, \
//                __FILE__, __LINE__, __FUNCTION__, \
//                ##args ); \
//    }while(0)
//
//  #define DEBUG_WARNING(fmt, args...) \
//    do{ \
//        time_t t; \
//        struct tm *p; \
//        struct timeval tv; \
//        gettimeofday (&tv , NULL); \
//        time(&t); \
//        p = localtime(&t); \
//        fprintf(stdout, "\033[40;33;m%.4d/%.2d/%.2d %.2d:%.2d:%.2d:%.6d [WARNING] %s:%d %s() "fmt"\n\033[5m", \
//                (1900+p->tm_year), (1+p->tm_mon),  p->tm_mday, \
//                p->tm_hour, p->tm_min, p->tm_sec, tv.tv_usec, \
//                __FILE__, __LINE__, __FUNCTION__, \
//                ##args ); \
//    }while(0)
//
//  #define DEBUG_ERROR(fmt, args...) \
//    do{ \
//        time_t t; \
//        struct tm *p; \
//        struct timeval tv; \
//        gettimeofday (&tv , NULL); \
//        time(&t); \
//        p = localtime(&t); \
//        fprintf(stderr, "\033[40;31;m%.4d/%.2d/%.2d %.2d:%.2d:%.2d:%.6d [ERROR] %s:%d %s() "fmt"\n\033[5m", \
//                (1900+p->tm_year), (1+p->tm_mon),  p->tm_mday, \
//                p->tm_hour, p->tm_min, p->tm_sec, tv.tv_usec, \
//                __FILE__, __LINE__, __FUNCTION__, \
//                ##args ); \
//    }while(0)
//#else
//  #define DEBUG_PRINT(fmt, args...) 
//  #define DEBUG_WARNING(fmt, args...) 
//  #define DEBUG_ERROR(fmt, args...)
//#endif

#define ThrowWandException(wand) \
{ \
    char *description; \
    ExceptionType severity; \
    description=MagickGetException(wand,&severity); \
    LOG_PRINT(LOG_ERROR, "%s %s %lu %s",GetMagickModule(),description); \
    description=(char *) MagickRelinquishMemory(description); \
}

