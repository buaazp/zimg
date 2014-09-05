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
 * @file zhttpd.c
 * @brief http protocol parse functions.
 * @author 招牌疯子 zp@buaa.us
 * @version 3.0.0
 * @date 2014-08-14
 */

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include "zhttpd.h"
#include "zimg.h"
#include "zmd5.h"
#include "zutil.h"
#include "zlog.h"
#include "zdb.h"
#include "zaccess.h"
#include "cjson/cJSON.h"

typedef struct {
    evhtp_request_t *req;
    thr_arg_t *thr_arg;
    char address[16];
    int partno;
    int succno;
    int check_name;
} mp_arg_t;

int zimg_etag_set(evhtp_request_t *request, char *buff, size_t len);
zimg_headers_conf_t * conf_get_headers(const char *hdr_str);
static int zimg_headers_add(evhtp_request_t *req, zimg_headers_conf_t *hcf);
void free_headers_conf(zimg_headers_conf_t *hcf);
static evthr_t * get_request_thr(evhtp_request_t *request);
static int print_headers(evhtp_header_t * header, void * arg); 
void add_info(MagickWand *im, evhtp_request_t *req);
void dump_request_cb(evhtp_request_t *req, void *arg);
void echo_request_cb(evhtp_request_t *req, void *arg);
void post_request_cb(evhtp_request_t *req, void *arg);
void get_request_cb(evhtp_request_t *req, void *arg);
void admin_request_cb(evhtp_request_t *req, void *arg);
void info_request_cb(evhtp_request_t *req, void *arg);

static const char * post_error_list[] = {
    "Internal error.",
    "File type not support.",
    "Request method error.",
    "Access error.",
    "Request body parse error.",
    "Content-Length error.",
    "Content-Type error.",
    "File too large.",
    "Request url illegal.",
    "Image not existed."
};

static const char * method_strmap[] = {
    "GET",
    "HEAD",
    "POST",
    "PUT",
    "DELETE",
    "MKCOL",
    "COPY",
    "MOVE",
    "OPTIONS",
    "PROPFIND",
    "PROPATCH",
    "LOCK",
    "UNLOCK",
    "TRACE",
    "CONNECT",
    "PATCH",
    "UNKNOWN",
};

/**
 * @brief zimg_etag_set set zimg response etag
 *
 * @param request the request of evhtp
 * @param buff response body buffer
 * @param len body buffer len
 *
 * @return 1 for set true and 2 for 304
 */
int zimg_etag_set(evhtp_request_t *request, char *buff, size_t len)
{
    int result = 1;
    LOG_PRINT(LOG_DEBUG, "Begin to Caculate MD5...");
    md5_state_t mdctx;
    md5_byte_t md_value[16];
    char md5sum[33];
    int i;
    int h, l;
    md5_init(&mdctx);
    md5_append(&mdctx, (const unsigned char*)(buff), len);
    md5_finish(&mdctx, md_value);

    for(i=0; i<16; ++i)
    {
        h = md_value[i] & 0xf0;
        h >>= 4;
        l = md_value[i] & 0x0f;
        md5sum[i * 2] = (char)((h >= 0x0 && h <= 0x9) ? (h + 0x30) : (h + 0x57));
        md5sum[i * 2 + 1] = (char)((l >= 0x0 && l <= 0x9) ? (l + 0x30) : (l + 0x57));
    }
    md5sum[32] = '\0';
    LOG_PRINT(LOG_DEBUG, "md5: %s", md5sum);

    const char *etag_var = evhtp_header_find(request->headers_in, "If-None-Match");
    LOG_PRINT(LOG_DEBUG, "If-None-Match: %s", etag_var);
    if(etag_var == NULL)
    {
        evhtp_headers_add_header(request->headers_out, evhtp_header_new("Etag", md5sum, 0, 1));
    }
    else
    {
        if(strncmp(md5sum, etag_var, 32) == 0)
        {
            result = 2;
        }
        else
        {
            evhtp_headers_add_header(request->headers_out, evhtp_header_new("Etag", md5sum, 0, 1));
        }
    }
    return result;
}

/**
 * @brief conf_get_headers get headers from conf
 *
 * @param hdr_str zimg conf string
 *
 * @return zimg_headers_conf
 */
zimg_headers_conf_t * conf_get_headers(const char *hdr_str)
{
    if(hdr_str == NULL)
        return NULL;
    zimg_headers_conf_t *hdrconf = (zimg_headers_conf_t *)malloc(sizeof(zimg_headers_conf_t));
    if(hdrconf == NULL)
        return NULL;
    hdrconf->n = 0;
    hdrconf->headers = NULL;
    size_t hdr_len = strlen(hdr_str); 
    char *hdr = (char *)malloc(hdr_len);
    if(hdr == NULL)
    {
        return NULL;
    }
    strncpy(hdr, hdr_str, hdr_len);
    char *start = hdr, *end;
    while(start <= hdr+hdr_len)
    {
        end = strchr(start, ';');
        end = (end) ? end : hdr+hdr_len;
        char *key = start;
        char *value = strchr(key, ':');
        size_t key_len = value - key;
        if (value)
        {
            zimg_header_t *this_header = (zimg_header_t *)malloc(sizeof(zimg_header_t));
            if (this_header == NULL) {
                start = end + 1;
                continue;
            }
            (void) memset(this_header, 0, sizeof(zimg_header_t));
            size_t value_len;
            value++;
            value_len = end - value;

            strncpy(this_header->key, key, key_len);
            strncpy(this_header->value, value, value_len);

            zimg_headers_t *headers = (zimg_headers_t *)malloc(sizeof(zimg_headers_t));
            if (headers == NULL) {
                start = end + 1;
                continue;
            }

            headers->value = this_header;
            headers->next = hdrconf->headers;
            hdrconf->headers = headers;
            hdrconf->n++;
        }
        start = end + 1;
    }
    free(hdr);
    return hdrconf;
}

/**
 * @brief zimg_headers_add add header to evhtp request from conf
 *
 * @param req the evhtp request
 * @param hcf zimg_header_conf
 *
 * @return 1 for OK and -1 for fail
 */
static int zimg_headers_add(evhtp_request_t *req, zimg_headers_conf_t *hcf)
{
    if(hcf == NULL) return -1;
    zimg_headers_t *headers = hcf->headers;
    LOG_PRINT(LOG_DEBUG, "headers: %d", hcf->n);

    while(headers)
    {
        evhtp_headers_add_header(req->headers_out, evhtp_header_new(headers->value->key, headers->value->value, 1, 1));
        headers = headers->next;
    }
    return 1;
}

/**
 * @brief free_headers_conf release zimg_headers_conf
 *
 * @param hcf the hcf
 */
void free_headers_conf(zimg_headers_conf_t *hcf)
{
    if(hcf == NULL)
        return;
    zimg_headers_t *headers = hcf->headers;
    while(headers)
    {
        hcf->headers = headers->next;
        free(headers->value);
        free(headers);
        headers = hcf->headers;
    }
    free(hcf);
}

/**
 * @brief get_request_thr get the request's thread
 *
 * @param request the evhtp request
 *
 * @return the thread dealing with the request
 */
static evthr_t * get_request_thr(evhtp_request_t *request)
{
    evhtp_connection_t * htpconn;
    evthr_t            * thread;

    htpconn = evhtp_request_get_connection(request);
    thread  = htpconn->thread;

    return thread;
}
/**
 * @brief guess_type It returns a HTTP type by guessing the file type.
 *
 * @param type Input a file type.
 *
 * @return Const string of type.
 */
/*
static const char * guess_type(const char *type)
{
	const struct table_entry *ent;
	for (ent = &content_type_table[0]; ent->extension; ++ent) {
		if (!evutil_ascii_strcasecmp(ent->extension, type))
			return ent->content_type;
	}
	return "application/misc";
}
*/

/* Try to guess a good content-type for 'path' */
/**
 * @brief guess_content_type Likes guess_type, but it process a whole path of file.
 *
 * @param path The path of a file you want to guess type.
 *
 * @return The string of type.
 */
/*
static const char * guess_content_type(const char *path)
{
	const char *last_period, *extension;
	const struct table_entry *ent;
	last_period = strrchr(path, '.');
	if (!last_period || strchr(last_period, '/'))
		goto not_found;
	extension = last_period + 1;
	for (ent = &content_type_table[0]; ent->extension; ++ent) {
		if (!evutil_ascii_strcasecmp(ent->extension, extension))
			return ent->content_type;
	}

not_found:
	return "application/misc";
}
*/

/**
 * @brief print_headers It displays all headers and values.
 *
 * @param header The header of a request.
 * @param arg The evbuff you want to store the k-v string.
 *
 * @return It always return 1 for success.
 */
static int print_headers(evhtp_header_t * header, void * arg) 
{
    evbuf_t * buf = arg;

    evbuffer_add(buf, header->key, header->klen);
    evbuffer_add(buf, ": ", 2);
    evbuffer_add(buf, header->val, header->vlen);
    evbuffer_add(buf, "\r\n", 2);
	return 1;
}

void add_info(MagickWand *im, evhtp_request_t *req)
{
    MagickSizeType size = MagickGetImageSize(im);
    unsigned long width = MagickGetImageWidth(im);
    unsigned long height = MagickGetImageHeight(im);
    size_t quality = MagickGetImageCompressionQuality(im);
    quality = (quality == 0 ? 100 : quality);
    char *format = MagickGetImageFormat(im);

    //{"ret":true,"info":{"size":195135,"width":720,"height":480,"quality":75,"format":"JPEG"}}
    cJSON *j_ret = cJSON_CreateObject();
    cJSON *j_ret_info = cJSON_CreateObject();
    cJSON_AddBoolToObject(j_ret, "ret", 1);
    cJSON_AddNumberToObject(j_ret_info, "size", size);
    cJSON_AddNumberToObject(j_ret_info, "width", width);
    cJSON_AddNumberToObject(j_ret_info, "height", height);
    cJSON_AddNumberToObject(j_ret_info, "quality", quality);
    cJSON_AddStringToObject(j_ret_info, "format", format);
    cJSON_AddItemToObject(j_ret, "info", j_ret_info);
    char *ret_str_unformat = cJSON_PrintUnformatted(j_ret);
    LOG_PRINT(LOG_DEBUG, "ret_str_unformat: %s", ret_str_unformat);
    evbuffer_add_printf(req->buffer_out, "%s", ret_str_unformat);
    cJSON_Delete(j_ret);
    free(ret_str_unformat);
    free(format);
}

/**
 * @brief dump_request_cb The callback of a dump request.
 *
 * @param req The request you want to dump.
 * @param arg It is not useful.
 */
void dump_request_cb(evhtp_request_t *req, void *arg)
{
    const char *uri = req->uri->path->full;

	//switch (evhtp_request_t_get_command(req)) {
    int req_method = evhtp_request_get_method(req);
    if(req_method >= 16)
        req_method = 16;

	LOG_PRINT(LOG_DEBUG, "Received a %s request for %s", method_strmap[req_method], uri);
    evbuffer_add_printf(req->buffer_out, "uri : %s\r\n", uri);
    evbuffer_add_printf(req->buffer_out, "query : %s\r\n", req->uri->query_raw);
    evhtp_headers_for_each(req->uri->query, print_headers, req->buffer_out);
    evbuffer_add_printf(req->buffer_out, "Method : %s\n", method_strmap[req_method]);
    evhtp_headers_for_each(req->headers_in, print_headers, req->buffer_out);

	evbuf_t *buf = req->buffer_in;;
	puts("Input data: <<<");
	while (evbuffer_get_length(buf)) {
		int n;
		char cbuf[128];
		n = evbuffer_remove(buf, cbuf, sizeof(buf)-1);
		if (n > 0)
			(void) fwrite(cbuf, 1, n, stdout);
	}
	puts(">>>");

    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", settings.server_name, 0, 1));
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Content-Type", "text/plain", 0, 0));
    evhtp_send_reply(req, EVHTP_RES_OK);
}

/**
 * @brief echo_cb Callback function of echo test.
 *
 * @param req The request of a test url.
 * @param arg It is not useful.
 */
void echo_cb(evhtp_request_t *req, void *arg)
{
    evbuffer_add_printf(req->buffer_out, "<html><body><h1>zimg works!</h1></body></html>");
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", settings.server_name, 0, 1));
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Content-Type", "text/html", 0, 0));
    evhtp_send_reply(req, EVHTP_RES_OK);
}

int on_header_field(multipart_parser* p, const char *at, size_t length)
{
    char *header_name = (char *)malloc(length+1);
    snprintf(header_name, length+1, "%s", at);
    LOG_PRINT(LOG_DEBUG, "header_name %d %s: ", length, header_name);
    free(header_name);
    return 0;
}

int on_header_value(multipart_parser* p, const char *at, size_t length)
{
    mp_arg_t *mp_arg = (mp_arg_t *)multipart_parser_get_data(p);
    char *filename = strnstr(at, "filename=", length);
    char *nameend = NULL;
    if(filename)
    {
        filename += 9;
        if(filename[0] == '\"')
        {
            filename++;
            nameend = strnchr(filename, '\"', length-(filename-at));
            if(!nameend)
                mp_arg->check_name = -1;
            else
            {
                nameend[0] = '\0';
                char fileType[32];
                LOG_PRINT(LOG_DEBUG, "File[%s]", filename);
                if(get_type(filename, fileType) == -1)
                {
                    LOG_PRINT(LOG_DEBUG, "Get Type of File[%s] Failed!", filename);
                    mp_arg->check_name = -1;
                }
                else
                {
                    LOG_PRINT(LOG_DEBUG, "fileType[%s]", fileType);
                    if(is_img(fileType) != 1)
                    {
                        LOG_PRINT(LOG_DEBUG, "fileType[%s] is Not Supported!", fileType);
                        mp_arg->check_name = -1;
                    }
                }
            }
        }
        if(filename[0] != '\0' && mp_arg->check_name == -1)
        {
            LOG_PRINT(LOG_ERROR, "%s fail post type", mp_arg->address);
            evbuffer_add_printf(mp_arg->req->buffer_out, 
                "<h1>File: %s</h1>\n"
                "<p>File type is not supported!</p>\n",
                filename
                );
        }
    }
    //multipart_parser_set_data(p, mp_arg);
    char *header_value = (char *)malloc(length+1);
    snprintf(header_value, length+1, "%s", at);
    LOG_PRINT(LOG_DEBUG, "header_value %d %s", length, header_value);
    free(header_value);
    return 0;
}

int on_chunk_data(multipart_parser* p, const char *at, size_t length)
{
    mp_arg_t *mp_arg = (mp_arg_t *)multipart_parser_get_data(p);
    mp_arg->partno++;
    if(mp_arg->check_name == -1)
    {
        mp_arg->check_name = 0;
        return 0;
    }
    if(length < 1)
        return 0;
    //multipart_parser_set_data(p, mp_arg);
    char md5sum[33];
    if(save_img(mp_arg->thr_arg, at, length, md5sum) == -1)
    {
        LOG_PRINT(LOG_DEBUG, "Image Save Failed!");
        LOG_PRINT(LOG_ERROR, "%s fail post save", mp_arg->address);
        evbuffer_add_printf(mp_arg->req->buffer_out, 
            "<h1>Failed!</h1>\n"
            "<p>File save failed!</p>\n"
            );
    }
    else
    {
        mp_arg->succno++;
        LOG_PRINT(LOG_INFO, "%s succ post pic:%s size:%d", mp_arg->address, md5sum, length);
        evbuffer_add_printf(mp_arg->req->buffer_out, 
            "<h1>MD5: %s</h1>\n"
            "Image upload successfully! You can get this image via this address:<br/><br/>\n"
            "<a href=\"/%s\">http://yourhostname:%d/%s</a>?w=width&h=height&p=proportion&g=isgray&x=crop_position_x&y=crop_position_y&q=quality\n",
            md5sum, md5sum, settings.port, md5sum
            );
    }
    return 0;
}

int json_return(evhtp_request_t *req, int err_no, const char *md5sum, int post_size)
{
    //json sample:
    //{"ret":true,"info":{"size":"1024", "md5":"edac35fd4b0059d3218f0630bc56a6f4"}}
    //{"ret":false,"error":{"code":"1","message":"\u9a8c\u8bc1\u5931\u8d25"}}
    cJSON *j_ret = cJSON_CreateObject();
    cJSON *j_ret_info = cJSON_CreateObject();
    if(err_no == -1)
    {
        cJSON_AddBoolToObject(j_ret, "ret", 1);
        cJSON_AddStringToObject(j_ret_info, "md5", md5sum);
        cJSON_AddNumberToObject(j_ret_info, "size", post_size);
        cJSON_AddItemToObject(j_ret, "info", j_ret_info);
    }
    else
    {
        cJSON_AddBoolToObject(j_ret, "ret", 0);
        cJSON_AddNumberToObject(j_ret_info, "code", err_no);
        LOG_PRINT(LOG_DEBUG, "post_error_list[%d]: %s", err_no, post_error_list[err_no]);
        cJSON_AddStringToObject(j_ret_info, "message", post_error_list[err_no]);
        cJSON_AddItemToObject(j_ret, "error", j_ret_info);
    }
    char *ret_str_unformat = cJSON_PrintUnformatted(j_ret);
    LOG_PRINT(LOG_DEBUG, "ret_str_unformat: %s", ret_str_unformat);
    evbuffer_add_printf(req->buffer_out, "%s", ret_str_unformat);
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Content-Type", "application/json", 0, 0));
    cJSON_Delete(j_ret);
    free(ret_str_unformat);
    return 0;
}

int binary_parse(evhtp_request_t *req, const char *content_type, const char *address, const char *buff, int post_size)
{
    int err_no = 0;
    if(is_img(content_type) != 1)
    {
        LOG_PRINT(LOG_DEBUG, "fileType[%s] is Not Supported!", content_type);
        LOG_PRINT(LOG_ERROR, "%s fail post type", address);
        err_no = 1;
        goto done;
    }
    char md5sum[33];
    LOG_PRINT(LOG_DEBUG, "Begin to Save Image...");
    evthr_t *thread = get_request_thr(req);
    thr_arg_t *thr_arg = (thr_arg_t *)evthr_get_aux(thread);
    if(save_img(thr_arg, buff, post_size, md5sum) == -1)
    {
        LOG_PRINT(LOG_DEBUG, "Image Save Failed!");
        LOG_PRINT(LOG_ERROR, "%s fail post save", address);
        goto done;
    }
    err_no = -1;
    LOG_PRINT(LOG_INFO, "%s succ post pic:%s size:%d", address, md5sum, post_size);
    json_return(req, err_no, md5sum, post_size);

done:
    return err_no;
}

int multipart_parse(evhtp_request_t *req, const char *content_type, const char *address, const char *buff, int post_size)
{
    int err_no = 0;
    char *boundary = NULL, *boundary_end = NULL;
    char *boundaryPattern = NULL;
    int boundary_len = 0;
    mp_arg_t *mp_arg = NULL;

    evbuffer_add_printf(req->buffer_out, 
            "<html>\n<head>\n"
            "<title>Upload Result</title>\n"
            "</head>\n"
            "<body>\n"
            );

    if(strstr(content_type, "boundary") == 0)
    {
        LOG_PRINT(LOG_DEBUG, "boundary NOT found!");
        LOG_PRINT(LOG_ERROR, "%s fail post parse", address);
        err_no = 6;
        goto done;
    }

    boundary = strchr(content_type, '=');
    boundary++;
    boundary_len = strlen(boundary);

    if(boundary[0] == '"') 
    {
        boundary++;
        boundary_end = strchr(boundary, '"');
        if (!boundary_end) 
        {
            LOG_PRINT(LOG_DEBUG, "Invalid boundary in multipart/form-data POST data");
            LOG_PRINT(LOG_ERROR, "%s fail post parse", address);
            err_no = 6;
            goto done;
        }
    } 
    else 
    {
        /* search for the end of the boundary */
        boundary_end = strpbrk(boundary, ",;");
    }
    if (boundary_end) 
    {
        boundary_end[0] = '\0';
        boundary_len = boundary_end-boundary;
    }

    LOG_PRINT(LOG_DEBUG, "boundary Find. boundary = %s", boundary);
    boundaryPattern = (char *)malloc(boundary_len + 3);
    if(boundaryPattern == NULL)
    {
        LOG_PRINT(LOG_DEBUG, "boundarypattern malloc failed!");
        LOG_PRINT(LOG_ERROR, "%s fail malloc", address);
        err_no = 0;
        goto done;
    }
    snprintf(boundaryPattern, boundary_len + 3, "--%s", boundary);
    LOG_PRINT(LOG_DEBUG, "boundaryPattern = %s, strlen = %d", boundaryPattern, (int)strlen(boundaryPattern));

    multipart_parser* parser = multipart_parser_init(boundaryPattern);
    if(!parser)
    {
        LOG_PRINT(LOG_DEBUG, "Multipart_parser Init Failed!");
        LOG_PRINT(LOG_ERROR, "%s fail post save", address);
        err_no = 0;
        goto done;
    }
    mp_arg = (mp_arg_t *)malloc(sizeof(mp_arg_t));
    if(!mp_arg)
    {
        LOG_PRINT(LOG_DEBUG, "Multipart_parser Arg Malloc Failed!");
        LOG_PRINT(LOG_ERROR, "%s fail post save", address);
        err_no = 0;
        goto done;
    }

    evthr_t *thread = get_request_thr(req);
    thr_arg_t *thr_arg = (thr_arg_t *)evthr_get_aux(thread);
    mp_arg->req = req;
    mp_arg->thr_arg = thr_arg;
    str_lcpy(mp_arg->address, address, 16);
    mp_arg->partno = 0;
    mp_arg->succno = 0;
    mp_arg->check_name = 0;
    multipart_parser_set_data(parser, mp_arg);
    multipart_parser_execute(parser, buff, post_size);
    multipart_parser_free(parser);

    if(mp_arg->succno == 0)
    {
        evbuffer_add_printf(req->buffer_out, "<h1>Upload Failed!</h1>\n");
    }

    evbuffer_add_printf(req->buffer_out, "</body>\n</html>\n");
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Content-Type", "text/html", 0, 0));
    err_no = -1;

done:
    free(boundaryPattern);
    free(mp_arg);
    return err_no;
}

/**
 * @brief post_request_cb The callback function of a POST request to upload a image.
 *
 * @param req The request with image buffer.
 * @param arg It is not useful.
 */
void post_request_cb(evhtp_request_t *req, void *arg)
{
    int post_size = 0;
    char *buff = NULL;
    int err_no = 0;
    int ret_json = 1;

    evhtp_connection_t *ev_conn = evhtp_request_get_connection(req);
    struct sockaddr *saddr = ev_conn->saddr;
    struct sockaddr_in *ss = (struct sockaddr_in *)saddr;
    char address[16];
    strncpy(address, inet_ntoa(ss->sin_addr), 16);

    int req_method = evhtp_request_get_method(req);
    if(req_method >= 16)
        req_method = 16;
    LOG_PRINT(LOG_DEBUG, "Method: %d", req_method);
    if(strcmp(method_strmap[req_method], "POST") != 0)
    {
        LOG_PRINT(LOG_DEBUG, "Request Method Not Support.");
        LOG_PRINT(LOG_INFO, "%s refuse post method", address);
        err_no = 2;
        goto err;
    }

    if(settings.up_access != NULL)
    {
        int acs = zimg_access_inet(settings.up_access, ss->sin_addr.s_addr);
        LOG_PRINT(LOG_DEBUG, "access check: %d", acs);
        if(acs == ZIMG_FORBIDDEN)
        {
            LOG_PRINT(LOG_DEBUG, "check access: ip[%s] forbidden!", address);
            LOG_PRINT(LOG_INFO, "%s refuse post forbidden", address);
            err_no = 3;
            goto forbidden;
        }
        else if(acs == ZIMG_ERROR)
        {
            LOG_PRINT(LOG_DEBUG, "check access: check ip[%s] failed!", address);
            LOG_PRINT(LOG_ERROR, "%s fail post access %s", address);
            err_no = 0;
            goto err;
        }
    }

    const char *content_len = evhtp_header_find(req->headers_in, "Content-Length");
    if(!content_len)
    {
        LOG_PRINT(LOG_DEBUG, "Get Content-Length error!");
        LOG_PRINT(LOG_ERROR, "%s fail post content-length", address);
        err_no = 5;
        goto err;
    }
    post_size = atoi(content_len);
    if(post_size <= 0)
    {
        LOG_PRINT(LOG_DEBUG, "Image Size is Zero!");
        LOG_PRINT(LOG_ERROR, "%s fail post empty", address);
        err_no = 5;
        goto err;
    }
    if(post_size > settings.max_size)
    {
        LOG_PRINT(LOG_DEBUG, "Image Size Too Large!");
        LOG_PRINT(LOG_ERROR, "%s fail post large", address);
        err_no = 7;
        goto err;
    }
    const char *content_type = evhtp_header_find(req->headers_in, "Content-Type");
    if(!content_type)
    {
        LOG_PRINT(LOG_DEBUG, "Get Content-Type error!");
        LOG_PRINT(LOG_ERROR, "%s fail post content-type", address);
        err_no = 6;
        goto err;
    }
	evbuf_t *buf;
    buf = req->buffer_in;
    buff = (char *)malloc(post_size);
    if(buff == NULL)
    {
        LOG_PRINT(LOG_DEBUG, "buff malloc failed!");
        LOG_PRINT(LOG_ERROR, "%s fail malloc buff", address);
        err_no = 0;
        goto err;
    }
    int rmblen, evblen;
    if(evbuffer_get_length(buf) <= 0)
    {
        LOG_PRINT(LOG_DEBUG, "Empty Request!");
        LOG_PRINT(LOG_ERROR, "%s fail post empty", address);
        err_no = 4;
        goto err;
    }
    while((evblen = evbuffer_get_length(buf)) > 0)
    {
        LOG_PRINT(LOG_DEBUG, "evblen = %d", evblen);
        rmblen = evbuffer_remove(buf, buff, evblen);
        LOG_PRINT(LOG_DEBUG, "rmblen = %d", rmblen);
        if(rmblen < 0)
        {
            LOG_PRINT(LOG_DEBUG, "evbuffer_remove failed!");
            LOG_PRINT(LOG_ERROR, "%s fail post parse", address);
            err_no = 4;
            goto err;
        }
    }
    if(strstr(content_type, "multipart/form-data") == NULL)
    {
        err_no = binary_parse(req, content_type, address, buff, post_size);
    }
    else
    {
        ret_json = 0;
        err_no = multipart_parse(req, content_type, address, buff, post_size);
    }
    if(err_no != -1)
    {
        goto err;
    }
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", settings.server_name, 0, 1));
    evhtp_send_reply(req, EVHTP_RES_OK);
    LOG_PRINT(LOG_DEBUG, "============post_request_cb() DONE!===============");
    goto done;

forbidden:
    json_return(req, err_no, NULL, 0);
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", settings.server_name, 0, 1));
    evhtp_send_reply(req, EVHTP_RES_OK);
    LOG_PRINT(LOG_DEBUG, "============post_request_cb() FORBIDDEN!===============");
    goto done;

err:
    if(ret_json == 0)
    {
        evbuffer_add_printf(req->buffer_out, "<h1>Upload Failed!</h1></body></html>"); 
        evhtp_headers_add_header(req->headers_out, evhtp_header_new("Content-Type", "text/html", 0, 0));
    }
    else
    {
        json_return(req, err_no, NULL, 0);
    }
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", settings.server_name, 0, 1));
    evhtp_send_reply(req, EVHTP_RES_OK);
    LOG_PRINT(LOG_DEBUG, "============post_request_cb() ERROR!===============");

done:
    free(buff);
}

/**
 * @brief get_request_cb The callback function of get a image request.
 *
 * @param req The request with a list of params and the md5 of image.
 * @param arg It is not useful.
 */
void get_request_cb(evhtp_request_t *req, void *arg)
{
    char *md5 = NULL, *fmt = NULL, *type = NULL;
    zimg_req_t *zimg_req = NULL;
	char *buff = NULL;
	size_t len;

    evhtp_connection_t *ev_conn = evhtp_request_get_connection(req);
    struct sockaddr *saddr = ev_conn->saddr;
    struct sockaddr_in *ss = (struct sockaddr_in *)saddr;
    char address[16];
    strncpy(address, inet_ntoa(ss->sin_addr), 16);

    int req_method = evhtp_request_get_method(req);
    if(req_method >= 16)
        req_method = 16;
    LOG_PRINT(LOG_DEBUG, "Method: %d", req_method);
    if(strcmp(method_strmap[req_method], "POST") == 0)
    {
        LOG_PRINT(LOG_DEBUG, "POST Request.");
        post_request_cb(req, NULL);
        return;
    }
	else if(strcmp(method_strmap[req_method], "GET") != 0)
    {
        LOG_PRINT(LOG_DEBUG, "Request Method Not Support.");
        LOG_PRINT(LOG_INFO, "%s refuse method", address);
        goto err;
    }

    if(settings.down_access != NULL)
    {
        int acs = zimg_access_inet(settings.down_access, ss->sin_addr.s_addr);
        LOG_PRINT(LOG_DEBUG, "access check: %d", acs);

        if(acs == ZIMG_FORBIDDEN)
        {
            LOG_PRINT(LOG_DEBUG, "check access: ip[%s] forbidden!", address);
            LOG_PRINT(LOG_INFO, "%s refuse get forbidden", address);
            goto forbidden;
        }
        else if(acs == ZIMG_ERROR)
        {
            LOG_PRINT(LOG_DEBUG, "check access: check ip[%s] failed!", address);
            LOG_PRINT(LOG_ERROR, "%s fail get access", address);
            goto err;
        }
    }

	const char *uri;
	uri = req->uri->path->full;

    if(strlen(uri) == 1 && uri[0] == '/')
    {
        LOG_PRINT(LOG_DEBUG, "Root Request.");
        int fd = -1;
        struct stat st;
        if((fd = open(settings.root_path, O_RDONLY)) == -1)
        {
            LOG_PRINT(LOG_DEBUG, "Root_page Open Failed. Return Default Page.");
            evbuffer_add_printf(req->buffer_out, "<html>\n<body>\n<h1>\nWelcome To zimg World!</h1>\n</body>\n</html>\n");
        }
        else
        {
            if (fstat(fd, &st) < 0)
            {
                /* Make sure the length still matches, now that we
                 * opened the file :/ */
                LOG_PRINT(LOG_DEBUG, "Root_page Length fstat Failed. Return Default Page.");
                evbuffer_add_printf(req->buffer_out, "<html>\n<body>\n<h1>\nWelcome To zimg World!</h1>\n</body>\n</html>\n");
            }
            else
            {
                evbuffer_add_file(req->buffer_out, fd, 0, st.st_size);
            }
        }
        //evbuffer_add_printf(req->buffer_out, "<html>\n </html>\n");
        evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", settings.server_name, 0, 1));
        evhtp_headers_add_header(req->headers_out, evhtp_header_new("Content-Type", "text/html", 0, 0));
        evhtp_send_reply(req, EVHTP_RES_OK);
        LOG_PRINT(LOG_DEBUG, "============get_request_cb() DONE!===============");
        LOG_PRINT(LOG_INFO, "%s succ root page", address);
        goto done;
    }

    if(strstr(uri, "favicon.ico"))
    {
        LOG_PRINT(LOG_DEBUG, "favicon.ico Request, Denied.");
        evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", settings.server_name, 0, 1));
        evhtp_headers_add_header(req->headers_out, evhtp_header_new("Content-Type", "text/html", 0, 0));
        zimg_headers_add(req, settings.headers);
        evhtp_send_reply(req, EVHTP_RES_OK);
        goto done;
    }
	LOG_PRINT(LOG_DEBUG, "Got a GET request for <%s>",  uri);

	/* Don't allow any ".."s in the path, to avoid exposing stuff outside
	 * of the docroot.  This test is both overzealous and underzealous:
	 * it forbids aceptable paths like "/this/one..here", but it doesn't
	 * do anything to prevent symlink following." */
	if (strstr(uri, ".."))
    {
        LOG_PRINT(LOG_DEBUG, "attempt to upper dir!");
        LOG_PRINT(LOG_INFO, "%s refuse directory", address);
		goto forbidden;
    }

    size_t md5_len = strlen(uri) + 1;
    md5 = (char *)malloc(md5_len);
    if(md5 == NULL)
    {
        LOG_PRINT(LOG_DEBUG, "md5 malloc failed!");
        LOG_PRINT(LOG_ERROR, "%s fail malloc", address);
        goto err;
    }
    if(uri[0] == '/')
        str_lcpy(md5, uri+1, md5_len);
    else
        str_lcpy(md5, uri, md5_len);
	LOG_PRINT(LOG_DEBUG, "md5 of request is <%s>",  md5);
    if(is_md5(md5) == -1)
    {
        LOG_PRINT(LOG_DEBUG, "Url is Not a zimg Request.");
        LOG_PRINT(LOG_INFO, "%s refuse url illegal", address);
        goto err;
    }
	/* This holds the content we're sending. */

    evthr_t *thread = get_request_thr(req);
    thr_arg_t *thr_arg = (thr_arg_t *)evthr_get_aux(thread);

    int width, height, proportion, gray, x, y, rotate, quality, sv;
    width = 0;
    height = 0;
    proportion = 1;
    gray = 0;
    x = -1;
    y = -1;
    rotate = 0;
    quality = 0;
    sv = 0;

    evhtp_kvs_t *params;
    params = req->uri->query;
    if(params != NULL)
    {
        if(settings.disable_args != 1)
        {
            const char *str_w = evhtp_kv_find(params, "w");
            const char *str_h = evhtp_kv_find(params, "h");
            width = (str_w) ? atoi(str_w) : 0;
            height = (str_h) ? atoi(str_h) : 0;

            if(width == 0 || height == 0)
            {
                proportion = 1;
            }
            else
            {
                const char *str_p = evhtp_kv_find(params, "p");
                proportion = (str_p) ? atoi(str_p) : 2;
            }

            const char *str_g = evhtp_kv_find(params, "g");
            gray = (str_g) ? atoi(str_g) : 0;

            const char *str_x = evhtp_kv_find(params, "x");
            const char *str_y = evhtp_kv_find(params, "y");
            x = (str_x) ? atoi(str_x) : -1;
            y = (str_y) ? atoi(str_y) : -1;

            if(x != -1 || y != -1)
            {
                proportion = 1;
            }

            const char *str_r = evhtp_kv_find(params, "r");
            rotate = (str_r) ? atoi(str_r) : 0;

            const char *str_q = evhtp_kv_find(params, "q");
            quality = (str_q) ? atoi(str_q) : 0;

            const char *str_f = evhtp_kv_find(params, "f");
            if(str_f)
            {
                size_t fmt_len = strlen(str_f) + 1;
                fmt = (char *)malloc(fmt_len);
                if(fmt != NULL)
                    str_lcpy(fmt, str_f, fmt_len);
                LOG_PRINT(LOG_DEBUG, "fmt = %s", fmt);
            }
        }

        if(settings.disable_type != 1)
        {
            const char *str_t = evhtp_kv_find(params, "t");
            if(str_t)
            {
                size_t type_len = strlen(str_t) + 1;
                type = (char *)malloc(type_len);
                if(type != NULL)
                    str_lcpy(type, str_t, type_len);
                LOG_PRINT(LOG_DEBUG, "type = %s", type);
            }
        }
    }
    else
    {
        sv = 1;
    }

    quality = (quality != 0 ? quality : settings.quality);
    zimg_req = (zimg_req_t *)malloc(sizeof(zimg_req_t)); 
    if(zimg_req == NULL)
    {
        LOG_PRINT(LOG_DEBUG, "zimg_req malloc failed!");
        LOG_PRINT(LOG_ERROR, "%s fail malloc", address);
        goto err;
    }
    zimg_req -> md5 = md5;
    zimg_req -> type = type;
    zimg_req -> width = width;
    zimg_req -> height = height;
    zimg_req -> proportion = proportion;
    zimg_req -> gray = gray;
    zimg_req -> x = x;
    zimg_req -> y = y;
    zimg_req -> rotate = rotate;
    zimg_req -> quality = quality;
    zimg_req -> fmt = (fmt != NULL ? fmt : settings.format);
    zimg_req -> sv = sv;
    zimg_req -> thr_arg = thr_arg;

    int get_img_rst = -1;
    get_img_rst = settings.get_img(zimg_req, req);

    if(get_img_rst == -1)
    {
        LOG_PRINT(LOG_DEBUG, "zimg Requset Get Image[MD5: %s] Failed!", zimg_req->md5);
        if(type)
            LOG_PRINT(LOG_ERROR, "%s fail pic:%s t:%s", address, md5, type);
        else
            LOG_PRINT(LOG_ERROR, "%s fail pic:%s w:%d h:%d p:%d g:%d x:%d y:%d r:%d q:%d f:%s",
                address, md5, width, height, proportion, gray, x, y, rotate, quality, zimg_req->fmt); 
        goto err;
    }
    if(get_img_rst == 2)
    {
        LOG_PRINT(LOG_DEBUG, "Etag Matched Return 304 EVHTP_RES_NOTMOD.");
        if(type)
            LOG_PRINT(LOG_INFO, "%s succ 304 pic:%s t:%s", address, md5, type);
        else
            LOG_PRINT(LOG_INFO, "%s succ 304 pic:%s w:%d h:%d p:%d g:%d x:%d y:%d r:%d q:%d f:%s",
                address, md5, width, height, proportion, gray, x, y, rotate, quality, zimg_req->fmt); 
        evhtp_send_reply(req, EVHTP_RES_NOTMOD);
        goto done;
    }

    len = evbuffer_get_length(req->buffer_out);
    LOG_PRINT(LOG_DEBUG, "get buffer length: %d", len);

    LOG_PRINT(LOG_DEBUG, "Got the File!");
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", settings.server_name, 0, 1));
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Content-Type", "image/jpeg", 0, 0));
    zimg_headers_add(req, settings.headers);
    evhtp_send_reply(req, EVHTP_RES_OK);
    if(type)
        LOG_PRINT(LOG_INFO, "%s succ pic:%s t:%s size:%d", address, md5, type, len); 
    else
        LOG_PRINT(LOG_INFO, "%s succ pic:%s w:%d h:%d p:%d g:%d x:%d y:%d r:%d q:%d f:%s size:%d", 
                address, md5, width, height, proportion, gray, x, y, rotate, quality, zimg_req->fmt, 
                len);
    LOG_PRINT(LOG_DEBUG, "============get_request_cb() DONE!===============");
    goto done;

forbidden:
    evbuffer_add_printf(req->buffer_out, "<html><body><h1>403 Forbidden!</h1></body></html>"); 
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", settings.server_name, 0, 1));
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Content-Type", "text/html", 0, 0));
    evhtp_send_reply(req, EVHTP_RES_FORBIDDEN);
    LOG_PRINT(LOG_DEBUG, "============get_request_cb() FORBIDDEN!===============");
    goto done;

err:
    evbuffer_add_printf(req->buffer_out, "<html><body><h1>404 Not Found!</h1></body></html>");
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", settings.server_name, 0, 1));
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Content-Type", "text/html", 0, 0));
    evhtp_send_reply(req, EVHTP_RES_NOTFOUND);
    LOG_PRINT(LOG_DEBUG, "============get_request_cb() ERROR!===============");

done:
    free(fmt);
    free(md5);
    free(type);
    free(zimg_req);
    free(buff);
}

// remove a image http://127.0.0.1:4869/admin?md5=5f189d8ec57f5a5a0d3dcba47fa797e2&t=1
/**
 * @brief admin_request_cb the callback function of admin requests
 *
 * @param req the evhtp request
 * @param arg the arg of request
 */
void admin_request_cb(evhtp_request_t *req, void *arg)
{
    char md5[33];

    evhtp_connection_t *ev_conn = evhtp_request_get_connection(req);
    struct sockaddr *saddr = ev_conn->saddr;
    struct sockaddr_in *ss = (struct sockaddr_in *)saddr;
    char address[16];
    strncpy(address, inet_ntoa(ss->sin_addr), 16);

    int req_method = evhtp_request_get_method(req);
    if(req_method >= 16)
        req_method = 16;
    LOG_PRINT(LOG_DEBUG, "Method: %d", req_method);
    if(strcmp(method_strmap[req_method], "POST") == 0)
    {
        LOG_PRINT(LOG_DEBUG, "POST Request.");
        post_request_cb(req, NULL);
        return;
    }
	else if(strcmp(method_strmap[req_method], "GET") != 0)
    {
        LOG_PRINT(LOG_DEBUG, "Request Method Not Support.");
        LOG_PRINT(LOG_INFO, "%s refuse method", address);
        goto err;
    }

    if(settings.admin_access != NULL)
    {
        int acs = zimg_access_inet(settings.admin_access, ss->sin_addr.s_addr);
        LOG_PRINT(LOG_DEBUG, "access check: %d", acs);

        if(acs == ZIMG_FORBIDDEN)
        {
            LOG_PRINT(LOG_DEBUG, "check access: ip[%s] forbidden!", address);
            LOG_PRINT(LOG_INFO, "%s refuse admin forbidden", address);
            goto forbidden;
        }
        else if(acs == ZIMG_ERROR)
        {
            LOG_PRINT(LOG_DEBUG, "check access: check ip[%s] failed!", address);
            LOG_PRINT(LOG_ERROR, "%s fail admin access", address);
            goto err;
        }
    }

	const char *uri;
	uri = req->uri->path->full;

    if(strstr(uri, "favicon.ico"))
    {
        LOG_PRINT(LOG_DEBUG, "favicon.ico Request, Denied.");
        evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", settings.server_name, 0, 1));
        evhtp_headers_add_header(req->headers_out, evhtp_header_new("Content-Type", "text/html", 0, 0));
        zimg_headers_add(req, settings.headers);
        evhtp_send_reply(req, EVHTP_RES_OK);
        return;
    }

	LOG_PRINT(LOG_DEBUG, "Got a Admin request for <%s>",  uri);
    int t;
    evhtp_kvs_t *params;
    params = req->uri->query;
    if(!params)
    {
        LOG_PRINT(LOG_DEBUG, "Admin Root Request.");
        int fd = -1;
        struct stat st;
        //TODO: use admin_path
        if((fd = open(settings.admin_path, O_RDONLY)) == -1)
        {
            LOG_PRINT(LOG_DEBUG, "Admin_page Open Failed. Return Default Page.");
            evbuffer_add_printf(req->buffer_out, "<html>\n<body>\n<h1>\nWelcome To zimg Admin!</h1>\n</body>\n</html>\n");
        }
        else
        {
            if (fstat(fd, &st) < 0)
            {
                /* Make sure the length still matches, now that we
                 * opened the file :/ */
                LOG_PRINT(LOG_DEBUG, "Root_page Length fstat Failed. Return Default Page.");
                evbuffer_add_printf(req->buffer_out, "<html>\n<body>\n<h1>\nWelcome To zimg Admin!</h1>\n</body>\n</html>\n");
            }
            else
            {
                evbuffer_add_file(req->buffer_out, fd, 0, st.st_size);
            }
        }
        //evbuffer_add_printf(req->buffer_out, "<html>\n </html>\n");
        evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", settings.server_name, 0, 1));
        evhtp_headers_add_header(req->headers_out, evhtp_header_new("Content-Type", "text/html", 0, 0));
        evhtp_send_reply(req, EVHTP_RES_OK);
        LOG_PRINT(LOG_DEBUG, "============admin_request_cb() DONE!===============");
        LOG_PRINT(LOG_INFO, "%s succ admin root page", address);
        return;
    }
    else
    {
        const char *str_md5, *str_t;
        str_md5 = evhtp_kv_find(params, "md5");
        if(str_md5 == NULL)
        {
            LOG_PRINT(LOG_DEBUG, "md5() = NULL return");
            goto err;
        }

        str_lcpy(md5, str_md5, sizeof(md5));
        if(is_md5(md5) == -1)
        {
            LOG_PRINT(LOG_DEBUG, "Admin Request MD5 Error.");
            LOG_PRINT(LOG_INFO, "%s refuse admin url illegal", address);
            goto err;
        }
        str_t = evhtp_kv_find(params, "t");
        LOG_PRINT(LOG_DEBUG, "md5() = %s; t() = %s;", str_md5, str_t);
        t = (str_t) ? atoi(str_t) : 0;
    }

    evthr_t *thread = get_request_thr(req);
    thr_arg_t *thr_arg = (thr_arg_t *)evthr_get_aux(thread);

    int admin_img_rst = -1;
    admin_img_rst = settings.admin_img(req, thr_arg, md5, t);

    if(admin_img_rst == -1)
    {
        evbuffer_add_printf(req->buffer_out, 
            "<html><body><h1>Admin Command Failed!</h1> \
            <p>MD5: %s</p> \
            <p>Command Type: %d</p> \
            <p>Command Failed.</p> \
            </body></html>",
            md5, t);
        LOG_PRINT(LOG_ERROR, "%s fail admin pic:%s t:%d", address, md5, t); 
        evhtp_headers_add_header(req->headers_out, evhtp_header_new("Content-Type", "text/html", 0, 0));
    }
    else if(admin_img_rst == 2)
    {
        evbuffer_add_printf(req->buffer_out, 
            "<html><body><h1>Admin Command Failed!</h1> \
            <p>MD5: %s</p> \
            <p>Command Type: %d</p> \
            <p>Image Not Found.</p> \
            </body></html>",
            md5, t);
        LOG_PRINT(LOG_ERROR, "%s 404 admin pic:%s t:%d", address, md5, t); 
        evhtp_headers_add_header(req->headers_out, evhtp_header_new("Content-Type", "text/html", 0, 0));
    }
    else
    {
        LOG_PRINT(LOG_INFO, "%s succ admin pic:%s t:%d", address, md5, t); 
    }
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", settings.server_name, 0, 1));
    evhtp_send_reply(req, EVHTP_RES_OK);
    LOG_PRINT(LOG_DEBUG, "============admin_request_cb() DONE!===============");
    return;

forbidden:
    evbuffer_add_printf(req->buffer_out, "<html><body><h1>403 Forbidden!</h1></body></html>"); 
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", settings.server_name, 0, 1));
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Content-Type", "text/html", 0, 0));
    evhtp_send_reply(req, EVHTP_RES_FORBIDDEN);
    LOG_PRINT(LOG_DEBUG, "============admin_request_cb() FORBIDDEN!===============");
    return;

err:
    evbuffer_add_printf(req->buffer_out, "<html><body><h1>404 Not Found!</h1></body></html>");
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", settings.server_name, 0, 1));
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Content-Type", "text/html", 0, 0));
    evhtp_send_reply(req, EVHTP_RES_NOTFOUND);
    LOG_PRINT(LOG_DEBUG, "============admin_request_cb() ERROR!===============");
    return;
}

void info_request_cb(evhtp_request_t *req, void *arg)
{
    char md5[33];
    int err_no = 0;

    evhtp_connection_t *ev_conn = evhtp_request_get_connection(req);
    struct sockaddr *saddr = ev_conn->saddr;
    struct sockaddr_in *ss = (struct sockaddr_in *)saddr;
    char address[16];
    strncpy(address, inet_ntoa(ss->sin_addr), 16);

    int req_method = evhtp_request_get_method(req);
    if(req_method >= 16)
        req_method = 16;
    LOG_PRINT(LOG_DEBUG, "Method: %d", req_method);
	if(strcmp(method_strmap[req_method], "GET") != 0)
    {
        err_no = 2;
        LOG_PRINT(LOG_DEBUG, "Request Method Not Support.");
        LOG_PRINT(LOG_INFO, "%s refuse info method", address);
        goto err;
    }

    evthr_t *thread = get_request_thr(req);
    thr_arg_t *thr_arg = (thr_arg_t *)evthr_get_aux(thread);

    evhtp_kvs_t *params;
    params = req->uri->query;
    if(params == NULL)
    {
        err_no = 8;
        LOG_PRINT(LOG_DEBUG, "zimg Requset Get Info Failed! No params");
        LOG_PRINT(LOG_INFO, "%s refuse info params", address);
        goto err;
    }

    const char *str_md5;
    str_md5 = evhtp_kv_find(params, "md5");
    if(str_md5 == NULL)
    {
        err_no = 8;
        LOG_PRINT(LOG_DEBUG, "md5() = NULL return");
        LOG_PRINT(LOG_INFO, "%s refuse info params", address);
        goto err;
    }

    str_lcpy(md5, str_md5, sizeof(md5));
    if(is_md5(md5) == -1)
    {
        err_no = 8;
        LOG_PRINT(LOG_DEBUG, "Admin Request MD5 Error.");
        LOG_PRINT(LOG_INFO, "%s refuse info md5", address);
        goto err;
    }

    int info_img_rst = -1;
    info_img_rst = settings.info_img(req, thr_arg, md5);

    if(info_img_rst == 0)
    {
        err_no = 9;
        LOG_PRINT(LOG_DEBUG, "zimg Requset Get Image[MD5: %s] Info Failed!", md5);
        LOG_PRINT(LOG_ERROR, "%s refuse info 404", address); 
        goto err;
    }
    else if(info_img_rst == -1)
    {
        err_no = 0;
        LOG_PRINT(LOG_DEBUG, "zimg Requset Get Image[MD5: %s] Info Failed!", md5);
        LOG_PRINT(LOG_ERROR, "%s fail info pic:%s", address, md5); 
        goto err;
    }

    LOG_PRINT(LOG_INFO, "%s succ info pic:%s", address, md5); 
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", settings.server_name, 0, 1));
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Content-Type", "application/json", 0, 0));
    evhtp_send_reply(req, EVHTP_RES_OK);
    LOG_PRINT(LOG_DEBUG, "============info_request_cb() SUCC!===============");
    goto done;

err:
    json_return(req, err_no, NULL, 0);
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", settings.server_name, 0, 1));
    evhtp_send_reply(req, EVHTP_RES_OK);
    LOG_PRINT(LOG_DEBUG, "============info_request_cb() ERROR!===============");

done:
    return;
}


