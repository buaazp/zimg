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
#include "zimg.h"
#include "zmd5.h"

int local_set(zreq_t *zreq) {
    LOG_PRINT(LOG_DEBUG, "Begin to Caculate MD5...");
    md5_state_t mdctx;
    md5_byte_t md_value[16];
    int i;
    int h, l;
    md5_init(&mdctx);
    md5_append(&mdctx, (const unsigned char*)(zreq->buff_in->buff),
               zreq->buff_in->len);
    md5_finish(&mdctx, md_value);

    zreq->key = (char *)malloc(33);
    if (zreq->key == NULL) {
        return api_err_malloc;
    }
    for (i = 0; i < 16; ++i) {
        h = md_value[i] & 0xf0;
        h >>= 4;
        l = md_value[i] & 0x0f;
        zreq->key[i * 2] = (char)((h >= 0x0 && h <= 0x9) ? (h + 0x30) : (h + 0x57));
        zreq->key[i * 2 + 1] = (char)((l >= 0x0 && l <= 0x9) ? (l + 0x30) : (l + 0x57));
    }
    zreq->key[32] = '\0';
    LOG_PRINT(LOG_DEBUG, "%s get file %s", zreq->ip, zreq->key);

    //caculate 2-level path
    int lvl1 = str_hash(zreq->key);
    int lvl2 = str_hash(zreq->key + 3);

    char save_path[512];
    snprintf(save_path, 512, "%s/%d/%d/%s", settings.img_path, lvl1, lvl2, zreq->key);
    LOG_PRINT(LOG_DEBUG, "save_path: %s", save_path);
    if (is_dir(save_path) != 1) {
        if (mk_dirs(save_path) == -1) {
            LOG_PRINT(LOG_DEBUG, "save_path[%s] Create Failed!", save_path);
            return api_err_mkdir;
        }
        LOG_PRINT(LOG_DEBUG, "save_path[%s] Create Finish.", save_path);
    }
    char save_name[512];
    snprintf(save_name, 512, "%s/0*0", save_path);
    LOG_PRINT(LOG_DEBUG, "save_name-->: %s", save_name);

    if (is_file(save_name) == 1) {
        LOG_PRINT(LOG_DEBUG, "Check File Exist. Needn't Save.");
        return api_ok;
    }

    if (new_img(zreq->buff_in->buff, zreq->buff_in->len, save_name) == -1) {
        LOG_PRINT(LOG_DEBUG, "Save Image[%s] Failed!", save_name);
        return api_err_new_file;
    }

    return api_ok;
}

int local_get(zreq_t *zreq) {
    zbuf_t *value = (zbuf_t *)malloc(sizeof(zbuf_t));
    if (value == NULL) return api_err_malloc;
    value->len = 0;
    value->buff = NULL;
    zreq->buff_in = value;

    LOG_PRINT(LOG_DEBUG, "local_get() %s start...", zreq->key);
    int lvl1 = str_hash(zreq->key);
    int lvl2 = str_hash(zreq->key + 3);
    char file_dir[512];
    snprintf(file_dir, 512, "%s/%d/%d/%s", settings.img_path, lvl1, lvl2, zreq->key);
    LOG_PRINT(LOG_DEBUG, "file_dir: %s", file_dir);

    if (is_dir(file_dir) == -1) {
        LOG_PRINT(LOG_DEBUG, "%s file is not existed", zreq->key);
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

int local_put(zreq_t *zreq) {
    //caculate 2-level path
    int lvl1 = str_hash(zreq->key);
    int lvl2 = str_hash(zreq->key + 3);

    char save_path[512];
    snprintf(save_path, 512, "%s/%d/%d/%s", settings.img_path, lvl1, lvl2,
             zreq->key);
    LOG_PRINT(LOG_DEBUG, "save_path: %s", save_path);
    if (is_dir(save_path) != 1) {
        if (mk_dirs(save_path) == -1) {
            LOG_PRINT(LOG_ERROR, "save_path [%s] create failed", save_path);
            return api_err_mkdir;
        }
        LOG_PRINT(LOG_DEBUG, "save_path [%s] create succ", save_path);
    }

    char save_name[512];
    if (settings.script_on == 1 && zreq->type != NULL) {
        snprintf(save_name, 512, "%s/t_%s", save_path, zreq->type);
    } else {
        if (zreq->width == 0 && zreq->height == 0 && zreq->proportion == 0) {
            LOG_PRINT(LOG_DEBUG, "return original image.");
            snprintf(save_name, 512, "%s/0*0", save_path);
        } else {
            char name[128];
            snprintf(name, 128, "%d*%d_p%d_g%d_%d*%d_r%d_q%d.%s",
                     zreq->width,
                     zreq->height,
                     zreq->proportion,
                     zreq->gray,
                     zreq->x, zreq->y,
                     zreq->rotate,
                     zreq->quality,
                     zreq->fmt);
            snprintf(save_name, 512, "%s/%s", save_path, name);
        }
    }
    LOG_PRINT(LOG_DEBUG, "save_name: %s", save_name);

    if (is_file(save_name) == 1) {
        LOG_PRINT(LOG_DEBUG, "file exists. needn't save");
        return api_ok;
    }

    if (new_img(zreq->buff_out->buff, zreq->buff_out->len, save_name) == -1) {
        LOG_PRINT(LOG_DEBUG, "save file [%s] failed", save_name);
        return api_err_new_file;
    }

    return api_ok;
}
