/*
 *   zimg - high performance image storage and processing system.
 *       http://zimg.buaa.us
 *
 *   Copyright (c) 2013-2015, Peter Zhao <zp@buaa.us>.
 *   All rights reserved.
 *
 *   Use and distribution licensed under the BSD license.
 *   See the LICENSE file for full text.
 *
 */

/**
 * @file zutil.c
 * @brief the util functions used by zimg.
 * @author 招牌疯子 zp@buaa.us
 * @version 3.2.0
 * @date 2015-10-24
 */

#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <ctype.h>
#include <dirent.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "zutil.h"
#include "zlog.h"

char * strnchr(const char *p, char c, size_t n);
char * strnstr(const char *s, const char *find, size_t slen);
size_t str_lcat(char *dst, const char *src, size_t size);
size_t str_lcpy(char *dst, const char *src, size_t size);
int bind_check(int port);
pid_t gettid(void);
int get_cpu_cores(void);
int get_type(const char *filename, char *type);
int is_file(const char *filename);
int is_img(const char *filename);
int is_dir(const char *path);
int is_special_dir(const char *path);
void get_file_path(const char *path, const char *file_name, char *file_path);
int mk_dir(const char *path);
int mk_dirs(const char *dir);
int mk_dirf(const char *filename);
int delete_file(const char *path);
int is_md5(char *s);
int str_hash(const char *str);
int gen_key(char *key, char *md5, ...);

/**
 * @brief strnchr find the pointer of a char in a string
 *
 * @param p the string
 * @param c the char
 * @param n find length
 *
 * @return the char position or 0
 */
char * strnchr(const char *p, char c, size_t n) {
    if (!p)
        return 0;

    while (n-- > 0) {
        if (*p == c)
            return ((char *)p);
        p++;
    }
    return 0;
}

/**
 * @brief strnstr find the sub string in a string
 *
 * @param s the string
 * @param find the sub string
 * @param slen find length
 *
 * @return the position of sub string or NULL
 */
char * strnstr(const char *s, const char *find, size_t slen) {
    char c, sc;
    size_t len;

    if ((c = *find++) != '\0') {
        len = strlen(find);
        do {
            do {
                if ((sc = *s++) == '\0' || slen-- < 1)
                    return (NULL);
            } while (sc != c);
            if (len > slen)
                return (NULL);
        } while (strncmp(s, find, len) != 0);
        s--;
    }
    return ((char *)s);
}

/*
 * '_cups_strlcat()' - Safely concatenate two strings.
 */
size_t                          /* O - Length of string */
str_lcat(char       *dst,       /* O - Destination string */
         const char  *src,       /* I - Source string */
         size_t      size) {     /* I - Size of destination string buffer */
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
         size_t          size) {     /* I - Size of destination string buffer */
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

/**
 * @brief bind_check check if the port is binded
 *
 * @param port the port
 *
 * @return 1 for OK or -1 for binded
 */
int bind_check(int port) {
    int mysocket, ret = -1;
    struct sockaddr_in my_addr;
    if ((mysocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return ret;
    }
    bzero(&my_addr, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(mysocket, (const struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
        if (errno == ECONNREFUSED) {
            ret = 1;
        }
    }
    close(mysocket);
    return ret;
}

/**
 * @brief gettid get pid
 *
 * @return pid
 */
pid_t gettid(void) {
    return syscall(SYS_gettid);
}

/**
 * @brief get_cpu_cores get the cpu cores of a server
 *
 * @return the cpu core number
 */
int get_cpu_cores(void) {
    return (int)sysconf(_SC_NPROCESSORS_CONF);
}

/**
 * @brief get_type It tell you the type of a file.
 *
 * @param filename The name of the file.
 * @param type Save the type string.
 *
 * @return 1 for success and -1 for fail.
 */
int get_type(const char *filename, char *type) {
    char *flag, *tmp;
    if ((flag = strchr(filename, '.')) == 0) {
        LOG_PRINT(LOG_DEBUG, "FileName [%s] Has No '.' in It.", filename);
        return -1;
    }
    while ((tmp = strchr(flag + 1, '.')) != 0) {
        flag = tmp;
    }
    flag++;
    str_lcpy(type, flag, 32);
    return 1;
}

/**
 * @brief is_file Check a filename is a file.
 *
 * @param filename The filename input.
 *
 * @return 1 for yes and -1 for no.
 */
int is_file(const char *filename) {
    struct stat st;
    if (stat(filename, &st) < 0) {
        LOG_PRINT(LOG_DEBUG, "File[%s] is Not Existed!", filename);
        return -1;
    }
    if (S_ISREG(st.st_mode)) {
        LOG_PRINT(LOG_DEBUG, "File[%s] is A File.", filename);
        return 1;
    }
    return -1;
}

/**
 * @brief is_img Check a file is a image we support(jpg, png, gif).
 *
 * @param filename The name of the file.
 *
 * @return  1 for success and 0 for fail.
 */
int is_img(const char *filename) {
    int isimg = -1;

    lua_getglobal(settings.L, "is_img");
    lua_pushstring(settings.L, filename);
    if (lua_pcall(settings.L, 1, 1, 0) != 0) {
        LOG_PRINT(LOG_WARNING, "lua is_img() failed!");
        return isimg;
    }
    isimg = (int)lua_tonumber(settings.L, -1);
    lua_pop(settings.L, 1);

    // char *imgType[] = {"jpg", "jpeg", "png", "gif", "webp"};
    // char *lower = (char *)malloc(strlen(filename) + 1);
    // if (lower == NULL) {
    //     return -1;
    // }
    // char *tmp;
    // int i;
    // for (i = 0; i < strlen(filename); i++) {
    //     lower[i] = tolower(filename[i]);
    // }
    // lower[strlen(filename)] = '\0';
    // for (i = 0; i < 5; i++) {
    //     LOG_PRINT(LOG_DEBUG, "compare %s - %s.", lower, imgType[i]);
    //     if ((tmp = strstr(lower, imgType[i])) == lower) {
    //         isimg = 1;
    //         break;
    //     }
    // }
    // free(lower);
    return isimg;
}

/**
 * @brief is_dir Check a path is a directory.
 *
 * @param path The path input.
 *
 * @return 1 for yes and -1 for no.
 */
int is_dir(const char *path) {
    struct stat st;
    if (stat(path, &st) < 0) {
        LOG_PRINT(LOG_DEBUG, "Path[%s] is Not Existed!", path);
        return -1;
    }
    if (S_ISDIR(st.st_mode)) {
        LOG_PRINT(LOG_DEBUG, "Path[%s] is A Dir.", path);
        return 1;
    } else
        return -1;
}

/**
 * @brief is_special_dir check if the path is a special path
 *
 * @param path the path want to check
 *
 * @return 1 for yes and -1 for not
 */
int is_special_dir(const char *path) {
    if (strcmp(path, ".") == 0 || strcmp(path, "..") == 0)
        return 1;
    else
        return -1;
}

/**
 * @brief get_file_path get the file's path
 *
 * @param path the file's parent path
 * @param file_name the file name
 * @param file_path the full path of the file
 */
void get_file_path(const char *path, const char *file_name, char *file_path) {
    strcpy(file_path, path);
    if (file_path[strlen(path) - 1] != '/')
        str_lcat(file_path, "/", PATH_MAX_SIZE);
    str_lcat(file_path, file_name, PATH_MAX_SIZE);
}


/**
 * @brief mk_dir It create a new directory with the path input.
 *
 * @param path The path you want to create.
 *
 * @return  1 for success and -1 for fail.
 */
int mk_dir(const char *path) {
    if (access(path, 0) == -1) {
        int status = mkdir(path, 0755);
        if (status == -1) {
            LOG_PRINT(LOG_DEBUG, "mkdir[%s] Failed!", path);
            return -1;
        }
        LOG_PRINT(LOG_DEBUG, "mkdir[%s] sucessfully!", path);
        return 1;
    } else {
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
int mk_dirs(const char *dir) {
    char tmp[256];
    str_lcpy(tmp, dir, sizeof(tmp));
    int i, len = strlen(tmp);
    if (tmp[len - 1] != '/')
        str_lcat(tmp, "/", sizeof(tmp));

    len = strlen(tmp);

    for (i = 1; i < len; i++) {
        if (tmp[i] == '/') {
            tmp[i] = 0;
            if (access(tmp, 0) != 0) {
                if (mkdir(tmp, 0755) == -1) {
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
int mk_dirf(const char *filename) {
    int ret = 1;
    if (access(filename, 0) == 0)
        return ret;
    size_t len = strlen(filename);
    char str[256];
    str_lcpy(str, filename, len);
    str[len] = '\0';
    char *end = str;
    char *start = strchr(end, '/');
    while (start) {
        end = start + 1;
        start = strchr(end, '/');
    }
    if (end != str) {
        str[end - str] = '\0';
        ret = mk_dirs(str);
    }
    return ret;
}

/**
 * @brief delete_file delete a file
 *
 * @param path the path of the file
 *
 * @return 1 for OK or -1 for fail
 */
int delete_file(const char *path) {
    DIR *dir;
    struct dirent *dir_info;
    char file_path[PATH_MAX_SIZE];
    int ret = -1;
    if (is_file(path) == 1) {
        remove(path);
        ret = 1;
    }
    if (is_dir(path) == 1) {
        if ((dir = opendir(path)) == NULL)
            return ret;
        ret = 1;
        while ((dir_info = readdir(dir)) != NULL) {
            get_file_path(path, dir_info->d_name, file_path);
            if (is_special_dir(dir_info->d_name) == 1)
                continue;
            ret = delete_file(file_path);
            if (ret == -1)
                break;
        }
        if (ret == 1)
            ret = rmdir(path);
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
int is_md5(char *s) {
    int rst = -1;
    int i = 0;
    for (; (s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'f') || (s[i] >= 'A' && s[i] <= 'F'); ++i) {
    }
    if (i == 32 && s[i] == '\0')
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
// static int htoi(char s[]) {
//     int i;
//     int n = 0;
//     if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
//         i = 2;
//     } else {
//         i = 0;
//     }
//     for (; (s[i] >= '0' && s[i] <= '9') || (s[i] >= 'a' && s[i] <= 'z') || (s[i] >= 'A' && s[i] <= 'Z'); ++i) {
//         if (s[i] > '9') {
//             n = 16 * n + (10 + s[i] - 'a');
//         } else {
//             n = 16 * n + (s[i] - '0');
//         }
//     }
//     return n;
// }


/**
 * @brief str_hash Hash algorithm of processing a md5 string.
 *
 * @param str The md5 string.
 *
 * @return The number less than 1024.
 */
int str_hash(const char *str) {
    char c[4];
    str_lcpy(c, str, 4);
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
int gen_key(char *key, char *md5, ...) {
    snprintf(key, CACHE_KEY_SIZE, "%s", md5);
    va_list arg_ptr;
    va_start(arg_ptr, md5);
    int argc = va_arg(arg_ptr, int);
    char tmp[CACHE_KEY_SIZE];
    //LOG_PRINT(LOG_DEBUG, "argc: %d", argc);
    if (argc > 1) {
        int i, argv;
        for (i = 0; i < argc - 1; i++) {
            argv = va_arg(arg_ptr, int);
            snprintf(tmp, CACHE_KEY_SIZE, "%s:%d", key, argv);
            snprintf(key, CACHE_KEY_SIZE, "%s", tmp);
            //LOG_PRINT(LOG_DEBUG, "arg[%d]: %d", i, argv);
        }
        char *fmt = va_arg(arg_ptr, char *);
        snprintf(tmp, CACHE_KEY_SIZE, "%s:%s", key, fmt);
        snprintf(key, CACHE_KEY_SIZE, "%s", tmp);
    }
    va_end(arg_ptr);
    LOG_PRINT(LOG_DEBUG, "key: %s", key);
    return 1;
}
