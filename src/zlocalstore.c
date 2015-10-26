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
 * @file zlocalstore.c
 * @brief Local storage interface.
 * @author 招牌疯子 zp@buaa.us
 * @version 3.2.0
 * @date 2015-10-24
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "zrestful.h"
#include "zlog.h"
#include "zutil.h"

int local_get(zreq_t *zreq) {
    zbuf_t *value = (zbuf_t *)malloc(sizeof(zbuf_t));
    if (value == NULL) return api_err_malloc;
    value->len = 0;
    value->buff = NULL;
    zreq->buff_in = value;

    LOG_PRINT(LOG_DEBUG, "local_get() %s start...", zreq->md5);
    int lvl1 = str_hash(zreq->md5);
    int lvl2 = str_hash(zreq->md5 + 3);
    char file_dir[512];
    snprintf(file_dir, 512, "%s/%d/%d/%s", settings.img_path, lvl1, lvl2, zreq->md5);
    LOG_PRINT(LOG_DEBUG, "file_dir: %s", file_dir);

    if (is_dir(file_dir) == -1) {
        LOG_PRINT(LOG_DEBUG, "%s file is not existed", zreq->md5);
        return api_err_not_exsit;
    }

    char file_path[512];
    snprintf(file_path, 512, "%s/0*0", file_dir);
    LOG_PRINT(LOG_DEBUG, "file_path: %s", file_path);

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        LOG_PRINT(LOG_DEBUG, "%s open failed: %s", file_path, strerror(errno));
        return api_err_open_file;
    }

    struct stat f_stat;
    fstat(fd, &f_stat);
    size_t len = f_stat.st_size;
    LOG_PRINT(LOG_DEBUG, "img_size = %d", len);
    if (len <= 0) {
        LOG_PRINT(LOG_DEBUG, "%s file is empty", file_path);
        return api_err_open_file;
    }
    char *buff = (char *)malloc(len);
    if (buff == NULL) {
        LOG_PRINT(LOG_ERROR, "buff malloc failed");
        return api_err_malloc;
    }
    value->buff = buff;

    size_t rlen = read(fd, buff, len);
    if (rlen == -1) {
        LOG_PRINT(LOG_DEBUG, "%s file read failed: %s", file_path, strerror(errno));
        return api_err_read_file;
    } else if (rlen < len) {
        LOG_PRINT(LOG_DEBUG, "%s file read not compeletly", file_path);
        return api_err_read_file;
    }
    value->len = len;

    return api_ok;
}