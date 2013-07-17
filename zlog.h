#include "zcommon.h"
#include "zspinlock.h"

#define MAX_LOGS        5                 /* 用户最多可打开日志数 */
 
#define LOG_INVALID     -1                  /* 无效的日志描述符(初始化定义) */
#define LOG_SYSTEM       0                  /* 系统日志(xmalloc, xabort.etc) */
#define LOG_USER         1                  /* 分配给用户的最小日志描述符 */
 
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
 
void    log_init();                         /* 初始化日志模块 */
 
int log_open(const char *path, const char* mode);           /* 打开用户日志文件 */
 
void    log_printf0(int log_id, int level, const char *fmt, ...);   /* 无位置信息 */
 
void    log_flush(int log_id);                      /* 将缓冲区数据写入日志 */
 
void    log_close(int log_id);                      /* 关闭日志文件 */

