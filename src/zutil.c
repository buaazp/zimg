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
 * @file zutil.c
 * @brief A suit of related functions.
 * @author 招牌疯子 zp@buaa.us
 * @version 1.0
 * @date 2013-07-19
 */

#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include "zutil.h"
#include "zlog.h"

//functions list
size_t str_lcat(char *dst, const char *src, size_t size);
size_t str_lcpy(char *dst, const char *src, size_t size);
int bind_check(int port);
pid_t gettid();
int get_cpu_cores();
static void kmp_init(const char *pattern, int pattern_size);
int kmp(const char *matcher, int mlen, const char *pattern, int plen);
int get_type(const char *filename, char *type);
int is_file(const char *filename);
int is_img(const char *filename);
int is_dir(const char *path);
int mk_dir(const char *path);
int mk_dirs(const char *dir);
int mk_dirf(const char *filename);
int is_md5(char *s);
static int htoi(char s[]);
int str_hash(const char *str);
int gen_key(char *key, char *md5, ...);

// this data is for KMP searching
static int pi[128];

/*
 * '_cups_strlcat()' - Safely concatenate two strings.
 */
size_t                          /* O - Length of string */
str_lcat(char       *dst,       /* O - Destination string */
        const char  *src,       /* I - Source string */
        size_t      size)       /* I - Size of destination string buffer */
{
    size_t    srclen;           /* Length of source string */
    size_t    dstlen;           /* Length of destination string */
    /*
    * Figure out how much room is left...
    */
    
    dstlen = strlen(dst);
    size   -= dstlen + 1;
    
    if (!size)
      return (dstlen);          /* No room, return immediately... */
    
    /*
    * Figure out how much room is needed...
    */
    
    srclen = strlen(src);
    /*
    * Copy the appropriate amount...
    */
    
    if (srclen > size)
      srclen = size;
    
    memcpy(dst + dstlen, src, srclen);
    dst[dstlen + srclen] = '\0';
    
    return (dstlen + srclen);
}

/*
 * '_cups_strlcpy()' - Safely copy two strings.
 */
size_t                              /* O - Length of string */
str_lcpy(char           *dst,       /* O - Destination string */
        const char      *src,       /* I - Source string */
        size_t          size)       /* I - Size of destination string buffer */
{
    size_t    srclen;               /* Length of source string */
    /*
    * Figure out how much room is needed...
    */
    size --;
    srclen = strlen(src);
    
    /*
    * Copy the appropriate amount...
    */
    if (srclen > size)
      srclen = size;
    
    memcpy(dst, src, srclen);
    dst[srclen] = '\0';
    
    return (srclen);
}

int bind_check(int port)
{
    int mysocket, ret = -1;
    struct sockaddr_in my_addr;
    if((mysocket = socket(AF_INET, SOCK_STREAM,0)) < 0)
    {
        return ret;
    }
    bzero(&my_addr,sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if(connect(mysocket, (const struct sockaddr *)&my_addr, sizeof(my_addr)) < 0)
    {
        if(errno == ECONNREFUSED)
        {
            ret = 1;
        }
    }
    close(mysocket);
    return ret;
}

pid_t gettid()
{
    return syscall(SYS_gettid);
}

int get_cpu_cores()
{
    return (int)sysconf(_SC_NPROCESSORS_CONF);
}

/* KMP for searching */
static void kmp_init(const char *pattern, int pattern_size)  // prefix-function
{
    pi[0] = 0;  // pi[0] always equals to 0 by defination
    int k = 0;  // an important pointer
    int q;
    for(q = 1; q < pattern_size; q++)  // find each pi[q] for pattern[q]
    {
        while(k>0 && pattern[k+1]!=pattern[q])
            k = pi[k];  // use previous prefixes to match pattern[0..q]

        if(pattern[k+1] == pattern[q]) // if pattern[0..(k+1)] is a prefix
            k++;             // let k = k + 1

        pi[q] = k;   // be ware, (0 <= k <= q), and (pi[k] < k)
    }
    // The worst-case time complexity of this procedure is O(pattern_size)
}

/**
 * @brief kmp The kmp algorithm.
 *
 * @param matcher The buffer.
 * @param mlen Buffer length.
 * @param pattern The pattern.
 * @param plen Pattern length.
 *
 * @return The place of pattern in buffer.
 */
int kmp(const char *matcher, int mlen, const char *pattern, int plen)
{
    // this function does string matching using the KMP algothm.
    // matcher and pattern are pointers to BINARY sequencies, while mlen
    // and plen are their lengths respectively.
    // if a match is found, return its position immediately.
    // return -1 if no match can be found.

    if(!mlen || !plen || mlen < plen) // take care of illegal parameters
        return -1;

    kmp_init(pattern, plen);  // prefix-function

    int i=0, j=0;
    while(i < mlen && j < plen)  // don't increase i and j at this level
    {
        if(matcher[i+j] == pattern[j])
            j++;
        else if(j == 0)  // dismatch: matcher[i] vs pattern[0]
            i++;
        else      // dismatch: matcher[i+j] vs pattern[j], and j>0
        {
            i = i + j - pi[j-1];  // i: jump forward by (j - pi[j-1])
            j = pi[j-1];          // j: reset to the proper position
        }
    }
    if(j == plen) // found a match!!
    {
        return i;
    }
    else          // if no match was found
        return -1;
}

/**
 * @brief get_type It tell you the type of a file.
 *
 * @param filename The name of the file.
 * @param type Save the type string.
 *
 * @return 1 for success and -1 for fail.
 */
int get_type(const char *filename, char *type)
{
    char *flag, *tmp;
    if((flag = strchr(filename, '.')) == 0)
    {
        LOG_PRINT(LOG_DEBUG, "FileName [%s] Has No '.' in It.", filename);
        return -1;
    }
    while((tmp = strchr(flag + 1, '.')) != 0)
    {
        flag = tmp;
    }
    flag++;
    strncpy(type, flag, 32);
    return 1;
}

/**
 * @brief is_file Check a filename is a file.
 *
 * @param filename The filename input.
 *
 * @return 1 for yes and -1 for no.
 */
int is_file(const char *filename)
{
    struct stat st;
    if(stat(filename, &st)<0)
    {
        LOG_PRINT(LOG_DEBUG, "File[%s] is Not Existed!", filename);
        return -1;
    }
    if(S_ISREG(st.st_mode))
    {
        LOG_PRINT(LOG_DEBUG, "File[%s] is A File.", filename);
        return 1;
    }
    else
        return -1;
}



/**
 * @brief is_img Check a file is a image we support(jpg, png, gif).
 *
 * @param filename The name of the file.
 *
 * @return  1 for success and 0 for fail.
 */
int is_img(const char *filename)
{
    char *imgType[] = {"jpg", "jpeg", "png", "gif"};
    char *lower= (char *)malloc(strlen(filename) + 1);
    if(lower == NULL)
    {
        return -1;
    }
    char *tmp;
    int i;
    int isimg = -1;
    for(i = 0; i < strlen(filename); i++)
    {
        lower[i] = tolower(filename[i]);
    }
    lower[strlen(filename)] = '\0';
    for(i = 0; i < 4; i++)
    {
        LOG_PRINT(LOG_DEBUG, "compare %s - %s.", lower, imgType[i]);
        if((tmp = strstr(lower, imgType[i])) == lower)
        {
            isimg = 1;
            break;
        }
    }
    free(lower);
    return isimg;
}

/**
 * @brief is_dir Check a path is a directory.
 *
 * @param path The path input.
 *
 * @return 1 for yes and -1 for no.
 */
int is_dir(const char *path)
{
    struct stat st;
    if(stat(path, &st)<0)
    {
        LOG_PRINT(LOG_DEBUG, "Path[%s] is Not Existed!", path);
        return -1;
    }
    if(S_ISDIR(st.st_mode))
    {
        LOG_PRINT(LOG_DEBUG, "Path[%s] is A Dir.", path);
        return 1;
    }
    else
        return -1;
}

/**
 * @brief mk_dir It create a new directory with the path input.
 *
 * @param path The path you want to create.
 *
 * @return  1 for success and -1 for fail.
 */
int mk_dir(const char *path)
{
    if(access(path, 0) == -1)
    {
        LOG_PRINT(LOG_DEBUG, "Begin to mk_dir()...");
        int status = mkdir(path, 0755);
        if(status == -1)
        {
            LOG_PRINT(LOG_DEBUG, "mkdir[%s] Failed!", path);
            return -1;
        }
        LOG_PRINT(LOG_DEBUG, "mkdir[%s] sucessfully!", path);
        return 1;
    }
    else
    {
        LOG_PRINT(LOG_DEBUG, "Path[%s] is Existed!", path);
        return -1;
    }
}

/**
 * @brief mk_dirs It creates a multi-level directory like ./img/330/28/557.
 *
 * @param dir The path of a multi-level directory.
 *
 * @return  1 for success and -1 for fail.
 */
int mk_dirs(const char *dir)
{
    char tmp[256];
    str_lcpy(tmp, dir, sizeof(tmp));
    int i, len = strlen(tmp);
    if(tmp[len-1] != '/')
        str_lcat(tmp, "/", sizeof(tmp));

    len = strlen(tmp);

    for(i=1; i<len; i++)
    {
        if(tmp[i] == '/')
        {
            tmp[i] = 0;
            if(access(tmp, 0) != 0)
            {
                if(mkdir(tmp, 0755) == -1)
                {
                    fprintf(stderr, "mk_dirs: tmp=%s\n", tmp);
                    return -1;
                }
            }
            tmp[i] = '/';
        }
    }
    return 1;
} 

/**
 * @brief mk_dirf Mkdirs for a full path filename
 *
 * @param filename the full path filename
 *
 * @return 1 for succ and -1 for fail
 */
int mk_dirf(const char *filename)
{
    int ret = 1;
    if(access(filename, 0) == 0)
        return ret;
    size_t len = strlen(filename);
    char str[256];
    strncpy(str, filename, len);
    str[len] = '\0';
    char *end = str;
    char *start = strchr(end, '/');
    while(start){
        end = start + 1;
        start = strchr(end, '/');
    }
    if(end != str)
    {
        str[end-str] = '\0';
        ret = mk_dirs(str);
    }
    return ret;
}

/**
 * @brief is_md5 Check the string is a md5 style.
 *
 * @param s The string.
 *
 * @return 1 for yes and -1 for no.
 */
int is_md5(char *s)
{
    int rst = -1;
    int i = 0;
    for (; (s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'f') || (s[i] >='A' && s[i] <= 'F');++i)
    {
    }
    if(i == 32 && s[i] == '\0')
        rst = 1;
    return rst;
}


/**
 * @brief htoi Exchange a hexadecimal string to a number.
 *
 * @param s[] The hexadecimal string.
 *
 * @return The number in the string.
 */
static int htoi(char s[])
{
    int i;
    int n = 0;
    if (s[0] == '0' && (s[1]=='x' || s[1]=='X'))
    {
        i = 2;
    }
    else
    {
        i = 0;
    }
    for (; (s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'z') || (s[i] >='A' && s[i] <= 'Z');++i)
    {
        if (s[i] > '9')
        {
            n = 16 * n + (10 + s[i] - 'a');
        }
        else
        {
            n = 16 * n + (s[i] - '0');
        }
    }
    return n;
}


/**
 * @brief str_hash Hash algorithm of processing a md5 string.
 *
 * @param str The md5 string.
 *
 * @return The number less than 1024.
 */
int str_hash(const char *str)
{
    char c[4];
    strncpy(c, str, 3);
    c[3] = '\0';
    //LOG_PRINT(LOG_DEBUG, "str = %s.", c);
    //int d = htoi(c);
    int d = strtol(c, NULL, 16);
    //LOG_PRINT(LOG_DEBUG, "str(3)_to_d = %d.", d);
    d = d / 4;
    //LOG_PRINT(LOG_DEBUG, "str(3)/4 = %d.", d);
    return d;
}


/**
 * @brief gen_key Generate storage key from md5 and other args.
 *
 * @param key The key string.
 * @param md5 The md5 string.
 * @param argc Count of args.
 * @param ... Args.
 *
 * @return Generate result.
 */
int gen_key(char *key, char *md5, ...)
{
    snprintf(key, CACHE_KEY_SIZE, "%s", md5);
    va_list arg_ptr;
    va_start(arg_ptr, md5);
    int argc = va_arg(arg_ptr, int);
    //LOG_PRINT(LOG_DEBUG, "argc: %d", argc);
    int i, argv;
    char tmp[CACHE_KEY_SIZE];
    for(i = 0; i<argc; i++)
    {
        argv = va_arg(arg_ptr, int);
        snprintf(tmp, CACHE_KEY_SIZE, "%s:%d", key, argv);
        snprintf(key, CACHE_KEY_SIZE, "%s", tmp);
        //LOG_PRINT(LOG_DEBUG, "arg[%d]: %d", i, argv);
    }
    va_end(arg_ptr);
    //LOG_PRINT(LOG_DEBUG, "key: %s", key);
    return 1;
}



