#include "zlog.h"

char *wday[]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
time_t timep;
struct tm *p;

//void DEBUG_PRINT(const char *fmt, ...)
void debug_print(char *__file, int __line, const char *fmt, ...)
{
    time(&timep);
    p=localtime(&timep);
    va_list args;
    va_start(args, fmt);
    fprintf(stdout, "[DEBUG] ");
    fprintf(stdout, "%d/%d/%d ", (1900+p->tm_year), (1+p->tm_mon),  p->tm_mday);
    fprintf(stdout, "%s %d:%d:%d ", wday[p->tm_wday], p->tm_hour, p->tm_min, p->tm_sec);
    fprintf(stdout, "%s:%d ",__FILE__,__LINE__);
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
    va_end(args);
}

void debug_error(const char *fmt, ...)
{
    time(&timep);
    p=localtime(&timep);
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[ERROR] ");
    fprintf(stderr, "%d/%d/%d ", (1900+p->tm_year), (1+p->tm_mon),  p->tm_mday);
    fprintf(stderr, "%s %d:%d:%d ", wday[p->tm_wday], p->tm_hour, p->tm_min, p->tm_sec);
    fprintf(stderr, "%s:%d-%s ",__FILE__,__LINE__,__FUNCTION__);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}
