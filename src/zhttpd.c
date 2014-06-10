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
 * @file zhttpd.c
 * @brief http protocol parse functions.
 * @author 招牌疯子 zp@buaa.us
 * @version 1.0
 * @date 2013-07-19
 */

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include "zhttpd.h"
#include "zimg.h"
#include "zutil.h"
#include "zlog.h"
#include "zdb.h"
#include "zaccess.h"

extern struct setting settings;

static evthr_t * get_request_thr(evhtp_request_t *request);
static const char * guess_type(const char *type);
static const char * guess_content_type(const char *path);
static int print_headers(evhtp_header_t * header, void * arg); 
void dump_request_cb(evhtp_request_t *req, void *arg);
void echo_cb(evhtp_request_t *req, void *arg);
void post_request_cb(evhtp_request_t *req, void *arg);
void send_document_cb(evhtp_request_t *req, void *arg);

static const struct table_entry {
	const char *extension;
	const char *content_type;
} content_type_table[] = {
	{ "txt", "text/plain" },
	{ "c", "text/plain" },
	{ "h", "text/plain" },
	{ "html", "text/html" },
	{ "htm", "text/htm" },
	{ "css", "text/css" },
	{ "gif", "image/gif" },
	{ "jpg", "image/jpeg" },
	{ "jpeg", "image/jpeg" },
	{ "png", "image/png" },
	{ "pdf", "application/pdf" },
	{ "ps", "application/postsript" },
	{ NULL, NULL },
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
static const char * guess_type(const char *type)
{
	const struct table_entry *ent;
	for (ent = &content_type_table[0]; ent->extension; ++ent) {
		if (!evutil_ascii_strcasecmp(ent->extension, type))
			return ent->content_type;
	}
	return "application/misc";
}

/* Try to guess a good content-type for 'path' */
/**
 * @brief guess_content_type Likes guess_type, but it process a whole path of file.
 *
 * @param path The path of a file you want to guess type.
 *
 * @return The string of type.
 */
static const char * guess_content_type(const char *path)
{
	const char *last_period, *extension;
	const struct table_entry *ent;
	last_period = strrchr(path, '.');
	if (!last_period || strchr(last_period, '/'))
		goto not_found; /* no exension */
	extension = last_period + 1;
	for (ent = &content_type_table[0]; ent->extension; ++ent) {
		if (!evutil_ascii_strcasecmp(ent->extension, extension))
			return ent->content_type;
	}

not_found:
	return "application/misc";
}

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

    //evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", "zimg/1.0.0 (Unix) (OpenSUSE/Linux)", 0, 0));
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", settings.server_name, 0, 0));
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
    //evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", "zimg/1.0.0 (Unix) (OpenSUSE/Linux)", 0, 0));
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", settings.server_name, 0, 0));
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Content-Type", "text/html", 0, 0));
    evhtp_send_reply(req, EVHTP_RES_OK);
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
    char *boundary = NULL, *boundary_end = NULL;
    int boundary_len = 0;
    char *fileName = NULL;
    char *boundaryPattern = NULL;
    char *buff = NULL;

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
            goto forbidden;
        }
        else if(acs == ZIMG_ERROR)
        {
            LOG_PRINT(LOG_DEBUG, "check access: check ip[%s] failed!", address);
            LOG_PRINT(LOG_ERROR, "%s fail post access %s", address);
            goto err;
        }
    }

    if(!evhtp_header_find(req->headers_in, "Content-Length"))
    {
        LOG_PRINT(LOG_DEBUG, "Get Content-Length error!");
        LOG_PRINT(LOG_ERROR, "%s fail post content-length", address);
        goto err;
    }
    const char *content_len = evhtp_header_find(req->headers_in, "Content-Length");
    post_size = atoi(content_len);
    if(!evhtp_header_find(req->headers_in, "Content-Type"))
    {
        LOG_PRINT(LOG_DEBUG, "Get Content-Type error!");
        LOG_PRINT(LOG_ERROR, "%s fail post content-type", address);
        goto err;
    }
    const char *content_type = evhtp_header_find(req->headers_in, "Content-Type");
    if(strstr(content_type, "multipart/form-data") == 0)
    {
        LOG_PRINT(LOG_DEBUG, "POST form error!");
        LOG_PRINT(LOG_ERROR, "%s fail post parse", address);
        goto err;
    }
    else if(strstr(content_type, "boundary") == 0)
    {
        LOG_PRINT(LOG_DEBUG, "boundary NOT found!");
        LOG_PRINT(LOG_ERROR, "%s fail post parse", address);
        goto err;
    }

    boundary = strchr(content_type, '=');
    boundary++;
    boundary_len = strlen(boundary);

    if (boundary[0] == '"') 
    {
        boundary++;
        boundary_end = strchr(boundary, '"');
        if (!boundary_end) 
        {
            LOG_PRINT(LOG_DEBUG, "Invalid boundary in multipart/form-data POST data");
            LOG_PRINT(LOG_ERROR, "%s fail post parse", address);
            goto err;
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
            
    // my own muti-part/form parse
	evbuf_t *buf;
    buf = req->buffer_in;
    buff = (char *)malloc(post_size);
    if(buff == NULL)
    {
        LOG_PRINT(LOG_DEBUG, "buff malloc failed!");
        LOG_PRINT(LOG_ERROR, "%s fail malloc buff", address);
        goto err;
    }
    int rmblen, evblen;
    int img_size = 0;

    if(evbuffer_get_length(buf) <= 0)
    {
        LOG_PRINT(LOG_DEBUG, "Empty Request!");
        LOG_PRINT(LOG_ERROR, "%s fail post empty", address);
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
            goto err;
        }
    }

    int start = -1, end = -1;
    const char *fileNamePattern = "filename=";
    const char *typePattern = "Content-Type";
    const char *quotePattern = "\"";
    const char *blankPattern = "\r\n";
    LOG_PRINT(LOG_DEBUG, "boundary = %s boundary_len = %d", boundary, boundary_len);
    //boundaryPattern = (char *)malloc(boundary_len + 3);
    boundaryPattern = (char *)malloc(boundary_len + 4);
    if(boundaryPattern == NULL)
    {
        LOG_PRINT(LOG_DEBUG, "boundarypattern malloc failed!");
        LOG_PRINT(LOG_ERROR, "%s fail malloc", address);
        goto err;
    }
    snprintf(boundaryPattern, boundary_len + 4, "\r\n--%s", boundary);
    //snprintf(boundaryPattern, boundary_len + 3, "--%s", boundary);
    LOG_PRINT(LOG_DEBUG, "boundaryPattern = %s, strlen = %d", boundaryPattern, (int)strlen(boundaryPattern));
    if((start = kmp(buff, post_size, fileNamePattern, strlen(fileNamePattern))) == -1)
    {
        LOG_PRINT(LOG_DEBUG, "Content-Disposition Not Found!");
        LOG_PRINT(LOG_ERROR, "%s fail post parse", address);
        goto err;
    }
    start += 9;
    if(buff[start] == '\"')
    {
        start++;
        if((end = kmp(buff+start, post_size-start, quotePattern, strlen(quotePattern))) == -1)
        {
            LOG_PRINT(LOG_DEBUG, "quote \" Not Found!");
            LOG_PRINT(LOG_ERROR, "%s fail post parse", address);
            goto err;
        }
    }
    else
    {
        if((end = kmp(buff+start, post_size-start, blankPattern, strlen(blankPattern))) == -1)
        {
            LOG_PRINT(LOG_DEBUG, "quote \\r\\n Not Found!");
            LOG_PRINT(LOG_ERROR, "%s fail post parse", address);
            goto err;
        }
    }
    fileName = (char *)malloc(end + 1);
    if(fileName == NULL)
    {
        LOG_PRINT(LOG_DEBUG, "filename malloc failed!");
        LOG_PRINT(LOG_ERROR, "%s fail malloc", address);
        goto err;
    }
    memcpy(fileName, buff+start, end);
    fileName[end] = '\0';
    LOG_PRINT(LOG_DEBUG, "fileName = %s", fileName);

    char fileType[32];
    if(get_type(fileName, fileType) == -1)
    {
        LOG_PRINT(LOG_DEBUG, "Get Type of File[%s] Failed!", fileName);
        LOG_PRINT(LOG_ERROR, "%s fail post type", address);
        goto err;
    }
    if(is_img(fileType) != 1)
    {
        LOG_PRINT(LOG_DEBUG, "fileType[%s] is Not Supported!", fileType);
        LOG_PRINT(LOG_ERROR, "%s fail post type", address);
        goto err;
    }

    end += start;

    if((start = kmp(buff+end, post_size-end, typePattern, strlen(typePattern))) == -1)
    {
        LOG_PRINT(LOG_DEBUG, "Content-Type Not Found!");
        LOG_PRINT(LOG_ERROR, "%s fail post parse", address);
        goto err;
    }
    start += end;
    LOG_PRINT(LOG_DEBUG, "start = %d", start);
    if((end =  kmp(buff+start, post_size-start, blankPattern, strlen(blankPattern))) == -1)
    {
        LOG_PRINT(LOG_DEBUG, "Image Not complete!");
        LOG_PRINT(LOG_ERROR, "%s fail post parse", address);
        goto err;
    }
    end += start;

    //by @momoplan: fixed some http tool's post bug.
    /*
       if((start = kmp(buff+end, post_size-end, "Content-Transfer-Encoding", strlen("Content-Transfer-Encoding"))) != -1)
       {
       start += end;
       if((end =  kmp(buff+start, post_size-start, blankPattern, strlen(blankPattern))) == -1)
       {
       LOG_PRINT(LOG_DEBUG, "Image Not complete!");
       LOG_PRINT(LOG_ERROR, "%s fail post parse", address);
       goto err;
       }
       end += start;
       }
       */

    LOG_PRINT(LOG_DEBUG, "end = %d", end);
    start = end + 4;
    LOG_PRINT(LOG_DEBUG, "start = %d", start);
    if((end = kmp(buff+start, post_size-start, boundaryPattern, strlen(boundaryPattern))) == -1)
    {
        LOG_PRINT(LOG_DEBUG, "Image Not complete!");
        LOG_PRINT(LOG_ERROR, "%s fail post parse", address);
        goto err;
    }
    end += start;
    LOG_PRINT(LOG_DEBUG, "end = %d", end);
    img_size = end - start;


    LOG_PRINT(LOG_DEBUG, "post_size = %d", post_size);
    LOG_PRINT(LOG_DEBUG, "img_size = %d", img_size);
    if(img_size <= 0)
    {
        LOG_PRINT(LOG_DEBUG, "Image Size is Zero!");
        LOG_PRINT(LOG_ERROR, "%s fail post empty", address);
        goto err;
    }

    char md5sum[33];

    LOG_PRINT(LOG_DEBUG, "Begin to Save Image...");

    evthr_t *thread = get_request_thr(req);
    thr_arg_t *thr_arg = (thr_arg_t *)evthr_get_aux(thread);
    if(save_img(thr_arg, buff+start, img_size, md5sum) == -1)
    {
        LOG_PRINT(LOG_DEBUG, "Image Save Failed!");
        LOG_PRINT(LOG_ERROR, "%s fail post save", address);
        goto err;
    }

    //libevhtp has bug with uri->authority->hostname, so zimg don't show hostname temporarily.
    //const char *host_name = req->uri->authority->hostname;
    //const int *host_port = req->uri->authority->port;
    //LOG_PRINT(LOG_DEBUG, "hostname: %s", req->uri->authority->hostname);
    //LOG_PRINT(LOG_DEBUG, "hostport: %d", host_port);
    evbuffer_add_printf(req->buffer_out, 
        "<html>\n<head>\n"
        "<title>Upload Successfully</title>\n"
        "</head>\n"
        "<body>\n"
        "<h1>MD5: %s</h1>\n"
        "Image upload successfully! You can get this image via this address:<br/><br/>\n"
        "http://yourhostname:%d/%s?w=width&h=height&g=isgray\n"
        "</body>\n</html>\n",
        md5sum, settings.port, md5sum
        );
    //evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", "zimg/1.0.0 (Unix) (OpenSUSE/Linux)", 0, 0));
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", settings.server_name, 0, 0));
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Content-Type", "text/html", 0, 0));
    evhtp_send_reply(req, EVHTP_RES_OK);
    LOG_PRINT(LOG_DEBUG, "============post_request_cb() DONE!===============");
    LOG_PRINT(LOG_INFO, "%s succ post pic:%s size:%d", address, md5sum, img_size);
    goto done;

forbidden:
    evbuffer_add_printf(req->buffer_out, "<html><body><h1>403 Forbidden!</h1></body><html>"); 
    //evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", "zimg/1.0.0 (Unix) (OpenSUSE/Linux)", 0, 0));
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", settings.server_name, 0, 0));
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Content-Type", "text/html", 0, 0));
    evhtp_send_reply(req, EVHTP_RES_FORBIDDEN);
    LOG_PRINT(LOG_DEBUG, "============post_request_cb() FORBIDDEN!===============");
    goto done;


err:
    evbuffer_add_printf(req->buffer_out, "<html><body><h1>Upload Failed!</h1></body><html>"); 
    //evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", "zimg/1.0.0 (Unix) (OpenSUSE/Linux)", 0, 0));
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", settings.server_name, 0, 0));
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Content-Type", "text/html", 0, 0));
    evhtp_send_reply(req, EVHTP_RES_SERVERR);
    LOG_PRINT(LOG_DEBUG, "============post_request_cb() ERROR!===============");

done:
    if(fileName)
        free(fileName);
    if(boundaryPattern)
        free(boundaryPattern);
    if(buff)
        free(buff);
}


/**
 * @brief send_document_cb The callback function of get a image request.
 *
 * @param req The request with a list of params and the md5 of image.
 * @param arg It is not useful.
 */
void send_document_cb(evhtp_request_t *req, void *arg)
{
    char *md5 = NULL;
	size_t len;
    zimg_req_t *zimg_req = NULL;
	char *buff = NULL;

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
	const char *rfull = req->uri->path->full;
	const char *rpath = req->uri->path->path;
	const char *rfile= req->uri->path->file;
	LOG_PRINT(LOG_DEBUG, "uri->path->full: %s",  rfull);
	LOG_PRINT(LOG_DEBUG, "uri->path->path: %s",  rpath);
	LOG_PRINT(LOG_DEBUG, "uri->path->file: %s",  rfile);

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
        evbuffer_add_printf(req->buffer_out, "<html>\n </html>\n");
        //evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", "zimg/1.0.0 (Unix) (OpenSUSE/Linux)", 0, 0));
        evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", settings.server_name, 0, 0));
        evhtp_headers_add_header(req->headers_out, evhtp_header_new("Content-Type", "text/html", 0, 0));
        evhtp_send_reply(req, EVHTP_RES_OK);
        LOG_PRINT(LOG_DEBUG, "============send_document_cb() DONE!===============");
        LOG_PRINT(LOG_INFO, "%s succ root page", address);
        goto done;
    }

    if(strstr(uri, "favicon.ico"))
    {
        LOG_PRINT(LOG_DEBUG, "favicon.ico Request, Denied.");
        //evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", "zimg/1.0.0 (Unix) (OpenSUSE/Linux)", 0, 0));
        evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", settings.server_name, 0, 0));
        evhtp_headers_add_header(req->headers_out, evhtp_header_new("Content-Type", "text/html", 0, 0));
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

    md5 = (char *)malloc(strlen(uri) + 1);
    if(md5 == NULL)
    {
        LOG_PRINT(LOG_DEBUG, "md5 malloc failed!");
        LOG_PRINT(LOG_ERROR, "%s fail malloc", address);
        goto err;
    }
    if(uri[0] == '/')
        strcpy(md5, uri+1);
    else
        strcpy(md5, uri);
	LOG_PRINT(LOG_DEBUG, "md5 of request is <%s>",  md5);
    if(is_md5(md5) == -1)
    {
        LOG_PRINT(LOG_DEBUG, "Url is Not a zimg Request.");
        LOG_PRINT(LOG_INFO, "%s refuse url illegal", address);
        goto err;
    }
	/* This holds the content we're sending. */

    int width, height, proportion, gray;
    evhtp_kvs_t *params;
    params = req->uri->query;
    if(!params)
    {
        width = 0;
        height = 0;
        proportion = 1;
        gray = 0;
    }
    else
    {
        const char *str_w, *str_h;
        str_w = evhtp_kv_find(params, "w");
        if(str_w == NULL)
        {
            str_w = "0";
        }
        str_h = evhtp_kv_find(params, "h");
        if(str_h == NULL)
        {
            str_h = "0";
        }
        LOG_PRINT(LOG_DEBUG, "w() = %s; h() = %s;", str_w, str_h);
        if(strcmp(str_w, "g") == 0 && strcmp(str_h, "w") == 0)
        {
            LOG_PRINT(LOG_DEBUG, "Love is Eternal.");
            evbuffer_add_printf(req->buffer_out, "<html>\n <head>\n"
                "  <title>Love is Eternal</title>\n"
                " </head>\n"
                " <body>\n"
                "  <h1>Single1024</h1>\n"
                "Since 2008-12-22, there left no room in my heart for another one.</br>\n"
                "</body>\n</html>\n"
                );
            evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", settings.server_name, 0, 0));
            evhtp_headers_add_header(req->headers_out, evhtp_header_new("Content-Type", "text/html", 0, 0));
            evhtp_send_reply(req, EVHTP_RES_OK);
            LOG_PRINT(LOG_DEBUG, "============send_document_cb() DONE!===============");
            goto done;
        }
        else
        {
            width = atoi(str_w);
            height = atoi(str_h);
            const char *str_p = evhtp_kv_find(params, "p");
            const char *str_g = evhtp_kv_find(params, "g");
            if(str_p)
            {
                proportion = atoi(str_p);
            }
            else
                proportion = 1;
            if(str_g)
            {
                gray = atoi(str_g);
            }
            else
                gray = 0;
        }
    }

    zimg_req = (zimg_req_t *)malloc(sizeof(zimg_req_t)); 
    if(zimg_req == NULL)
    {
        LOG_PRINT(LOG_DEBUG, "zimg_req malloc failed!");
        LOG_PRINT(LOG_ERROR, "%s fail malloc", address);
        goto err;
    }
    zimg_req -> md5 = md5;
    zimg_req -> width = width;
    zimg_req -> height = height;
    zimg_req -> proportion = proportion;
    zimg_req -> gray = gray;

    
    evthr_t *thread = get_request_thr(req);
    thr_arg_t *thr_arg = (thr_arg_t *)evthr_get_aux(thread);
    zimg_req -> thr_arg = thr_arg;

    int get_img_rst = -1;
    if(settings.mode == 1)
        get_img_rst = get_img(zimg_req, req);
        //get_img_rst = get_img2(zimg_req, req);
    else
        get_img_rst = get_img_mode_db(zimg_req, req);
        //get_img_rst = get_img_mode_db2(zimg_req, req);


    if(get_img_rst == -1)
    {
        LOG_PRINT(LOG_DEBUG, "zimg Requset Get Image[MD5: %s] Failed!", zimg_req->md5);
        LOG_PRINT(LOG_ERROR, "%s fail pic:%s w:%d h:%d p:%d g:%d", address, md5, width, height, proportion, gray);
        goto err;
    }

    len = evbuffer_get_length(req->buffer_out);
    LOG_PRINT(LOG_DEBUG, "get buffer length: %d", len);

    LOG_PRINT(LOG_DEBUG, "Got the File!");
    //evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", "zimg/1.0.0 (Unix) (OpenSUSE/Linux)", 0, 0));
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", settings.server_name, 0, 0));
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Content-Type", "image/jpeg", 0, 0));
    evhtp_send_reply(req, EVHTP_RES_OK);
    LOG_PRINT(LOG_INFO, "%s succ pic:%s w:%d h:%d p:%d g:%d size:%d", address, md5, width, height, proportion, gray, len);
    LOG_PRINT(LOG_DEBUG, "============send_document_cb() DONE!===============");
    goto done;

forbidden:
    evbuffer_add_printf(req->buffer_out, "<html><body><h1>403 Forbidden!</h1></body><html>"); 
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", settings.server_name, 0, 0));
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Content-Type", "text/html", 0, 0));
    evhtp_send_reply(req, EVHTP_RES_FORBIDDEN);
    LOG_PRINT(LOG_DEBUG, "============send_document_cb() FORBIDDEN!===============");
    goto done;

err:
    evbuffer_add_printf(req->buffer_out, "<html><body><h1>404 Not Found!</h1></body></html>");
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Server", settings.server_name, 0, 0));
    evhtp_headers_add_header(req->headers_out, evhtp_header_new("Content-Type", "text/html", 0, 0));
    evhtp_send_reply(req, EVHTP_RES_NOTFOUND);
    LOG_PRINT(LOG_DEBUG, "============send_document_cb() ERROR!===============");

done:
	if(buff)
		free(buff);
    if(zimg_req)
    {
        if(zimg_req->md5)
            free(zimg_req->md5);
        free(zimg_req);
    }
}

