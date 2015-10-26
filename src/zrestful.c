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
 * @file zrestful.c
 * @brief http restful handlers.
 * @author 招牌疯子 zp@buaa.us
 * @version 3.2.0
 * @date 2015-10-24
 */

#include "libevhtp/evhtp.h"
#include "cjson/cJSON.h"
#include "zcommon.h"
#include "zlog.h"
#include "zaccess.h"
#include "zrestful.h"
#include "zutil.h"
#include "zlocalstore.h"
#include "zimage.h"

const char *http_method_name[] = {
    "get",
    "head",
    "post",
    "put",
    "delete",
    "mkcol",
    "copy",
    "move",
    "options",
    "propfind",
    "propatch",
    "lock",
    "unlock",
    "trace",
    "connect",
    "patch",
    "unknown",
};

const int get_prefix_step = 8;

const char *api_error_msg[] = {
    "req succ",
    "image key illegal",
    "width or height too big",
    "access forbidden",
    "method forbidden",
    "check access fail",
    "image is not exsited",
    "memory malloc fail",
    "file open fail",
    "file read fail",
    "imagick error",
};

const int api_error_status[] = {
    EVHTP_RES_OK,
    EVHTP_RES_BADREQ,
    EVHTP_RES_BADREQ,
    EVHTP_RES_FORBIDDEN,
    EVHTP_RES_METHNALLOWED,
    EVHTP_RES_SERVERR,
    EVHTP_RES_SERVERR,
    EVHTP_RES_SERVERR,
    EVHTP_RES_SERVERR,
    EVHTP_RES_SERVERR,
};

void free_zbuf(zbuf_t *zbuff) {
    if (zbuff) {
        if (zbuff->buff) free(zbuff->buff);
        free(zbuff);
    }
    return;
}

zreq_t *new_zreq(evhtp_request_t *req) {
    zreq_t *zreq = (zreq_t *)malloc(sizeof(zreq_t));
    zreq->req = req;
    zreq->ip = NULL;
    zreq->type = NULL;
    zreq->fmt = NULL;
    zreq->thr_arg = NULL;
    zreq->buff_in = NULL;
    zreq->buff_out = NULL;
    return zreq;
}

int check_access(zreq_t *zreq, http_method method_want,
                 zimg_access_conf_t *conf) {
    evhtp_request_t *req = zreq->req;
    evhtp_connection_t *ev_conn = evhtp_request_get_connection(req);
    struct sockaddr *saddr = ev_conn->saddr;
    struct sockaddr_in *ss = (struct sockaddr_in *)saddr;
    const char *xff_address = evhtp_header_find(req->headers_in,
                              "X-Forwarded-For");
    if (xff_address) inet_aton(xff_address, &ss->sin_addr);
    zreq->ip = (char *)malloc(16);
    if (zreq->ip) strncpy(zreq->ip, inet_ntoa(ss->sin_addr), 16);

    int method = evhtp_request_get_method(req);
    if (method != method_want) {
        LOG_PRINT(LOG_INFO, "%s %s refuse: method %d forbidden", zreq->ip,
                  http_method_name[method_want], method);
        return api_err_method_forbidden;
    }

    if (conf != NULL) {
        int ret = zimg_access_inet(conf, ss->sin_addr.s_addr);
        if (ret == ZIMG_FORBIDDEN) {
            LOG_PRINT(LOG_INFO, "%s %s refuse: access forbidden", zreq->ip, http_method_name[method_want]);
            return api_err_access_forbidden;
        } else if (ret == ZIMG_ERROR) {
            LOG_PRINT(LOG_ERROR, "%s %s fail: check access fail", zreq->ip, http_method_name[method_want]);
            return api_err_check_access;
        }
    }
    return api_ok;
}

int parse_param(zreq_t *zreq) {
    char *key = zreq->req->uri->path->full;
    key += get_prefix_step;
    LOG_PRINT(LOG_DEBUG, "%s parsing key %s...", zreq->ip, key);
    if (is_md5(key) == -1) {
        return api_err_key_illegal;
    }

    size_t md5_len = strlen(key) + 1;
    zreq->md5 = (char *)malloc(md5_len);
    if (zreq->md5 != NULL) {
        str_lcpy(zreq->md5, key, md5_len);
        LOG_PRINT(LOG_DEBUG, "%s get md5 %s", zreq->ip, zreq->md5);
    }

    int width = 0;
    int height = 0;
    int proportion = 1;
    int gray = 0;
    int x = -1;
    int y = -1;
    int rotate = 0;
    int quality = 0;
    int sv = 0;

    evhtp_kvs_t *params = zreq->req->uri->query;
    if (params != NULL) {
        if (settings.disable_args != 1) {
            const char *str_w = evhtp_kv_find(params, "w");
            const char *str_h = evhtp_kv_find(params, "h");
            width = (str_w) ? atoi(str_w) : 0;
            height = (str_h) ? atoi(str_h) : 0;
            if (settings.max_pixel > 0) {
                if (width > settings.max_pixel || height > settings.max_pixel) {
                    LOG_PRINT(LOG_DEBUG, "%s get arg: %s w:%d h:%d limit: %d",
                              zreq->ip, zreq->md5, width, height,
                              settings.max_pixel);
                    return api_err_pixel_over;
                }
            }

            const char *str_p = evhtp_kv_find(params, "p");
            proportion = (str_p) ? atoi(str_p) : 1;

            const char *str_g = evhtp_kv_find(params, "g");
            gray = (str_g) ? atoi(str_g) : 0;

            const char *str_x = evhtp_kv_find(params, "x");
            const char *str_y = evhtp_kv_find(params, "y");
            x = (str_x) ? atoi(str_x) : -1;
            y = (str_y) ? atoi(str_y) : -1;

            if (x != -1 || y != -1) {
                proportion = 1;
            }

            const char *str_r = evhtp_kv_find(params, "r");
            rotate = (str_r) ? atoi(str_r) : 0;

            const char *str_q = evhtp_kv_find(params, "q");
            quality = (str_q) ? atoi(str_q) : 0;

            const char *str_f = evhtp_kv_find(params, "f");
            if (str_f) {
                size_t fmt_len = strlen(str_f) + 1;
                zreq->fmt = (char *)malloc(fmt_len);
                if (zreq->fmt != NULL) {
                    str_lcpy(zreq->fmt, str_f, fmt_len);
                    for (int i = 0; zreq->fmt[i]; i++) {
                        zreq->fmt[i] = tolower(zreq->fmt[i]);
                    }
                    LOG_PRINT(LOG_DEBUG, "%s get fmt %s", zreq->ip, zreq->fmt);
                }
            }
        }

        if (settings.disable_type != 1) {
            const char *str_t = evhtp_kv_find(params, "t");
            if (str_t) {
                size_t type_len = strlen(str_t) + 1;
                zreq->type = (char *)malloc(type_len);
                if (zreq->type != NULL) {
                    str_lcpy(zreq->type, str_t, type_len);
                    LOG_PRINT(LOG_DEBUG, "%s get type %s", zreq->ip, zreq->type);
                }
            }
        }
    } else {
        sv = 1;
    }

    quality = (quality != 0 ? quality : settings.quality);
    zreq -> width = width;
    zreq -> height = height;
    zreq -> proportion = proportion;
    zreq -> gray = gray;
    zreq -> x = x;
    zreq -> y = y;
    zreq -> rotate = rotate;
    zreq -> quality = quality;
    zreq -> sv = sv;
    return api_ok;
}

void free_zreq(zreq_t *zreq) {
    if (zreq) {
        if (zreq->ip) free(zreq->ip);
        if (zreq->type) free(zreq->type);
        if (zreq->fmt) free(zreq->fmt);
        free_zbuf(zreq->buff_in);
        free_zbuf(zreq->buff_out);
        free(zreq);
    }
    return;
}

void api_response(zreq_t *zreq, api_error_code ecode) {
    evhtp_request_t *req = zreq->req;
    if (ecode != api_ok) {
        const char *msg = api_error_msg[ecode];
        cJSON *j_ret = cJSON_CreateObject();
        cJSON_AddNumberToObject(j_ret, "code", ecode);
        cJSON_AddStringToObject(j_ret, "message", msg);
        char *ret_str_unformat = cJSON_PrintUnformatted(j_ret);
        LOG_PRINT(LOG_DEBUG, "ret_str_unformat: %s", ret_str_unformat);
        evhtp_headers_add_header(req->headers_out, evhtp_header_new(
                                     "Content-Type", "application/json", 0, 0));
        evbuffer_add_printf(req->buffer_out, "%s", ret_str_unformat);
        free(ret_str_unformat);
        cJSON_Delete(j_ret);
    } else {
        evbuffer_add_printf(req->buffer_out, "%s", "image body");
        evhtp_headers_add_header(req->headers_out, evhtp_header_new(
                                     "Content-Type", "text/html", 0, 0));
    }
    evhtp_headers_add_header(req->headers_out, evhtp_header_new(
                                 "Server", settings.server_name, 0, 1));
    evhtp_send_reply(req, api_error_status[ecode]);
    free_zreq(zreq);
    return;
}

void get_image_cb(evhtp_request_t *req, void *arg) {
    zreq_t *zreq = new_zreq(req);

    int ecode = check_access(zreq, GET, settings.down_access);
    if (ecode != api_ok) {
        LOG_PRINT(LOG_INFO, "%s get refuse: %s", zreq->ip, api_error_msg[ecode]);
        api_response(zreq, ecode);
        return;
    }

    ecode = parse_param(zreq);
    if (ecode != api_ok) {
        LOG_PRINT(LOG_INFO, "%s get refuse: %s", zreq->ip, api_error_msg[ecode]);
        api_response(zreq, ecode);
        return;
    }

    ecode = local_get(zreq);
    if (ecode != api_ok) {
        LOG_PRINT(LOG_INFO, "%s get fail: %s", zreq->ip, api_error_msg[ecode]);
        api_response(zreq, ecode);
        return;
    }

    // TODO: convert image
    evthr_t *thread = get_request_thr(req);
    thr_arg_t *thr_arg = (thr_arg_t *)evthr_get_aux(thread);
    zreq->thr_arg = thr_arg;

    ecode = convert2(zreq);
    if (ecode != api_ok) {
        LOG_PRINT(LOG_INFO, "%s get fail: %s", zreq->ip, api_error_msg[ecode]);
        api_response(zreq, ecode);
        return;
    }

    if (zreq->type)
        LOG_PRINT(LOG_INFO, "%s get succ: %s t:%s size:%d", zreq->ip, zreq->md5,
                  zreq->type, zreq->buff_in->len);
    else
        LOG_PRINT(LOG_INFO, "%s get succ: %s w:%d h:%d p:%d g:%d x:%d y:%d r:%d q:%d f:%s size: %d",
                  zreq->ip, zreq->md5, zreq->width, zreq->height,
                  zreq->proportion, zreq->gray, zreq->x, zreq->y, zreq->rotate,
                  zreq->quality, zreq->fmt, zreq->buff_in->len);
    api_response(zreq, api_ok);
    return;
}