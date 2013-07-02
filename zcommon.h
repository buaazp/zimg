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
#include <openssl/md5.h>

#define MAX_LINE 1024 

char _root_path[256];
char _img_path[256];
char _log_path[256];
int _port;
int _log;

#define DEBUG_PRINT(fmt, args...) \
    do{ \
        time_t t; \
        struct tm *p; \
        struct timeval tv; \
        struct timezone tz; \
        gettimeofday (&tv , &tz); \
        time(&t); \
        p = localtime(&t); \
        fprintf(stdout, "\033[40;32;m[DEBUG] %.4d/%.2d/%.2d %.2d:%.2d:%.2d:%.6d %s:%d %s() "fmt"\n\033[5m", \
                (1900+p->tm_year), (1+p->tm_mon),  p->tm_mday, \
                p->tm_hour, p->tm_min, p->tm_sec, tv.tv_usec, \
                __FILE__, __LINE__, __FUNCTION__, \
                ##args ); \
    }while(0)

#define DEBUG_WARNING(fmt, args...) \
    do{ \
        time_t t; \
        struct tm *p; \
        struct timeval tv; \
        struct timezone tz; \
        gettimeofday (&tv , &tz); \
        time(&t); \
        p = localtime(&t); \
        fprintf(stdout, "\033[40;33;m[WARNING] %.4d/%.2d/%.2d %.2d:%.2d:%.2d:%.6d %s:%d %s() "fmt"\n\033[5m", \
                (1900+p->tm_year), (1+p->tm_mon),  p->tm_mday, \
                p->tm_hour, p->tm_min, p->tm_sec, tv.tv_usec, \
                __FILE__, __LINE__, __FUNCTION__, \
                ##args ); \
    }while(0)



#define DEBUG_ERROR(fmt, args...) \
    do{ \
        time_t t; \
        struct tm *p; \
        struct timeval tv; \
        struct timezone tz; \
        gettimeofday (&tv , &tz); \
        time(&t); \
        p = localtime(&t); \
        fprintf(stderr, "\033[40;31;m[ERROR] %.4d/%.2d/%.2d %.2d:%.2d:%.2d:%.6d %s:%d %s() "fmt"\n\033[5m", \
                (1900+p->tm_year), (1+p->tm_mon),  p->tm_mday, \
                p->tm_hour, p->tm_min, p->tm_sec, tv.tv_usec, \
                __FILE__, __LINE__, __FUNCTION__, \
                ##args ); \
    }while(0)

#define ThrowWandException(wand) \
{ \
    char \
    *description; \
    \
    ExceptionType \
    severity; \
    \
    description=MagickGetException(wand,&severity); \
    (void) fprintf(stderr,"%s %s %lu %s\n",GetMagickModule(),description); \
    description=(char *) MagickRelinquishMemory(description); \
    exit(-1); \
}
