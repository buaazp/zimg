#include "zhttpd.h"
#include "zimg.h"

// this data is for KMP searching 
//int pi[128];
char uri_root[512];

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

/* Try to guess a good content-type for 'path' */
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

/* Callback used for the /dump URI, and for every non-GET request:
 * dumps all information to stdout and gives back a trivial 200 ok */
static void dump_request_cb(struct evhttp_request *req, void *arg)
{
	const char *cmdtype;
	struct evkeyvalq *headers;
	struct evkeyval *header;
	struct evbuffer *buf;

	switch (evhttp_request_get_command(req)) {
	case EVHTTP_REQ_GET: cmdtype = "GET"; break;
	case EVHTTP_REQ_POST: cmdtype = "POST"; break;
	case EVHTTP_REQ_HEAD: cmdtype = "HEAD"; break;
	case EVHTTP_REQ_PUT: cmdtype = "PUT"; break;
	case EVHTTP_REQ_DELETE: cmdtype = "DELETE"; break;
	case EVHTTP_REQ_OPTIONS: cmdtype = "OPTIONS"; break;
	case EVHTTP_REQ_TRACE: cmdtype = "TRACE"; break;
	case EVHTTP_REQ_CONNECT: cmdtype = "CONNECT"; break;
	case EVHTTP_REQ_PATCH: cmdtype = "PATCH"; break;
	default: cmdtype = "unknown"; break;
	}

	LOG_PRINT(LOG_INFO, "Received a %s request for %s",
	    cmdtype, evhttp_request_get_uri(req));
    LOG_PRINT(LOG_INFO, "Headers:");

	headers = evhttp_request_get_input_headers(req);
	for (header = headers->tqh_first; header;
	    header = header->next.tqe_next) {
		LOG_PRINT(LOG_INFO, "  %s: %s", header->key, header->value);
	}

	buf = evhttp_request_get_input_buffer(req);
	puts("Input data: <<<");
	while (evbuffer_get_length(buf)) {
		int n;
		char cbuf[128];
		n = evbuffer_remove(buf, cbuf, sizeof(buf)-1);
		if (n > 0)
			(void) fwrite(cbuf, 1, n, stdout);
	}
	puts(">>>");

	evhttp_send_reply(req, 200, "OK", NULL);
}



/* Callback used for the POST requset:
 * storage image data and gives back a trivial 200 ok */
static void post_request_cb(struct evhttp_request *req, void *arg)
{
    struct evbuffer *evb = NULL;
    struct evkeyvalq *headers;
    struct evkeyval *header;
    int postSize = 0;
    char *boundary = NULL, *boundary_end = NULL;
    int boundary_len = 0;
    
	LOG_PRINT(LOG_INFO, "Received a POST request for %s", evhttp_request_get_uri(req));
    LOG_PRINT(LOG_INFO, "Headers:");

	headers = evhttp_request_get_input_headers(req);
	for (header = headers->tqh_first; header; header = header->next.tqe_next) 
    {
		LOG_PRINT(LOG_INFO, "  %s: %s", header->key, header->value);
        if(strstr(header->key, "Content-Length") == header->key)
        {
            postSize = atoi(header->value);
        }
        else if(strstr(header->key, "Content-Type") == header->key)
        {
            if(strstr(header->value, "multipart/form-data") == 0)
            {
                LOG_PRINT(LOG_ERROR, "POST form error!");
                goto err;
            }
            else if(strstr(header->value, "boundary") == 0)
            {
                LOG_PRINT(LOG_ERROR, "boundary NOT found!");
                goto err;
            }
            else
            {
                boundary = strchr(header->value, '=');
                boundary++;
                boundary_len = strlen(boundary);

                if (boundary[0] == '"') 
                {
                    boundary++;
                    boundary_end = strchr(boundary, '"');
                    if (!boundary_end) 
                    {
                        LOG_PRINT(LOG_ERROR, "Invalid boundary in multipart/form-data POST data");
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

                LOG_PRINT(LOG_INFO, "boundary Find. boundary = %s", boundary);
            }
        }
	}
    /* 依靠evbuffer自己实现php处理函数 */
	struct evbuffer *buf;
    buf = evhttp_request_get_input_buffer(req);
    char *buff = (char *)malloc(postSize);
    char *fileName;
    char buffline[128];
    int rmblen, evblen;
    int imgSize = 0;
//    int wlen = 0;
//    int fd = -1;

    //************* print the binary data of img
    //puts("Input data: <<<");
    while((evblen = evbuffer_get_length(buf)) > 0)
    {
        LOG_PRINT(LOG_INFO, "evblen = %d", evblen);
        rmblen = evbuffer_remove(buf, buff, evblen);
        LOG_PRINT(LOG_INFO, "rmblen = %d", rmblen);
        if(rmblen < 0)
        {
            LOG_PRINT(LOG_ERROR, "evbuffer_remove failed!");
            goto err;
        }
        /*
        else
        {
            (void) fwrite(buff, 1, rmblen, stdout);
        }
        */
    }
    //puts(">>>");

    /*
    LOG_PRINT(LOG_INFO, "input data>>>");
    (void) fwrite(buff, 1, rmblen, stdout);
    printf("\n");
    LOG_PRINT(LOG_INFO, ">>>");
    */

    int start = -1, end = -1;
    char *fileNamePattern = "filename=";
    char *typePattern = "Content-Type";
    char *quotePattern = "\"";
    char *blankPattern = "\r\n";
    char *boundaryPattern = (char *)malloc(boundary_len + 4);
    sprintf(boundaryPattern, "\r\n--%s", boundary);
    /*
    LOG_PRINT(LOG_INFO, "boundaryPattern[%d] = %c", boundary_len + 3, boundaryPattern[boundary_len+3]);
    if(boundaryPattern[boundary_len+4] == '\0')
        LOG_PRINT(LOG_INFO, "boundaryPattern[%d] = %s", boundary_len + 4, "\\0");
    else
        LOG_PRINT(LOG_INFO, "boundaryPattern[%d] = %c", boundary_len + 4, boundaryPattern[boundary_len+4]);
    */
    LOG_PRINT(LOG_INFO, "boundaryPattern = %s, strlen = %d", boundaryPattern, (int)strlen(boundaryPattern));
    if((start = kmp(buff, postSize, fileNamePattern, strlen(fileNamePattern))) == -1)
    {
        LOG_PRINT(LOG_ERROR, "Content-Disposition Not Found!");
        goto err;
    }
    start += 9;
    if(buff[start] == '\"')
    {
        start++;
        if((end = kmp(buff+start, postSize-start, quotePattern, strlen(quotePattern))) == -1)
        {
            LOG_PRINT(LOG_ERROR, "quote \" Not Found!");
            goto err;
        }
    }
    else
    {
        if((end = kmp(buff+start, postSize-start, blankPattern, strlen(blankPattern))) == -1)
        {
            LOG_PRINT(LOG_ERROR, "quote \" Not Found!");
            goto err;
        }
    }
    fileName = (char *)malloc(end + 1);
    memcpy(fileName, buff+start, end);
    fileName[end] = '\0';
    LOG_PRINT(LOG_INFO, "fileName = %s", fileName);

    char fileType[32];
    if(get_type(fileName, fileType) == -1)
    {
        LOG_PRINT(LOG_ERROR, "Get Type of File[%s] Failed!", fileName);
        goto err;
    }
    if(is_img(fileType) != 1)
    {
        LOG_PRINT(LOG_ERROR, "fileType[%s] is Not Supported!", fileType);
        goto err;
    }

    end += start;

    if((start = kmp(buff+end, postSize-end, typePattern, strlen(typePattern))) == -1)
    {
        LOG_PRINT(LOG_ERROR, "Content-Type Not Found!");
        goto err;
    }
    start += end;
    LOG_PRINT(LOG_INFO, "start = %d", start);
    if((end =  kmp(buff+start, postSize-start, blankPattern, strlen(blankPattern))) == -1)
    {
        LOG_PRINT(LOG_ERROR, "Image Not complete!");
        goto err;
    }
    end += start;
    LOG_PRINT(LOG_INFO, "end = %d", end);
    start = end + 4;
    LOG_PRINT(LOG_INFO, "start = %d", start);
    if((end = kmp(buff+start, postSize-start, boundaryPattern, strlen(boundaryPattern))) == -1)
    {
        LOG_PRINT(LOG_ERROR, "Image Not complete!");
        goto err;
    }
    end += start;
    LOG_PRINT(LOG_INFO, "end = %d", end);
    imgSize = end - start;


    LOG_PRINT(LOG_INFO, "postSize = %d", postSize);
    LOG_PRINT(LOG_INFO, "imgSize = %d", imgSize);
    char md5sum[33];

    LOG_PRINT(LOG_INFO, "Begin to Save Image...");
    //if(save_img(fileType, buff+start, imgSize, md5sum) == -1)
    if(save_img(buff+start, imgSize, md5sum, fileType) == -1)
    {
        LOG_PRINT(LOG_ERROR, "Image Save Failed!");
        goto err;
    }

    evb = evbuffer_new();
    evbuffer_add_printf(evb, "<html>\n <head>\n"
        "  <title>%s</title>\n"
        " </head>\n"
        " <body>\n"
        "  <h1>MD5: %s</h1>\n",
        "Upload Successfully",
        md5sum);
    evbuffer_add_printf(evb, "</body></html>\n");
    evhttp_add_header(evhttp_request_get_output_headers(req),"Content-Type", "text/html");
    evhttp_send_reply(req, 200, "OK", evb);
    evbuffer_free(evb);
    LOG_PRINT(LOG_INFO, "============post_request_cb() DONE!===============");
    goto done;

err:
    evhttp_send_error(req, 500, "Image Upload Failed!");
    LOG_PRINT(LOG_INFO, "============post_request_cb() ERROR!===============");

done:
    if(fileName)
        free(fileName);
    if(boundaryPattern)
        free(boundaryPattern);
    if(buff)
        free(buff);
}


/* This callback gets invoked when we get any http request that doesn't match
 * any other callback.  Like any evhttp server callback, it has a simple job:
 * it must eventually call evhttp_send_error() or evhttp_send_reply().
 */
static void send_document_cb(struct evhttp_request *req, void *arg)
{
	struct evbuffer *evb = NULL;
	const char *uri = evhttp_request_get_uri(req);
	struct evhttp_uri *decoded = NULL;
	const char *path;
	char *decoded_path;
	char *whole_path = NULL;
	size_t len;
	int fd = -1;
	struct stat st;
    zimg_req_t *zimg_req = NULL;
	char *buff = NULL;

    if (evhttp_request_get_command(req) == EVHTTP_REQ_POST) 
    {
        LOG_PRINT(LOG_INFO, "POST --> post_request_cb(req, %s)", _img_path);
        post_request_cb(req, NULL);
        LOG_PRINT(LOG_INFO, "post_request_cb(req, %s) --> POST", _img_path);
        return;
    }
    else if(evhttp_request_get_command(req) != EVHTTP_REQ_GET){
        dump_request_cb(req, NULL);
        return;
    }

	/* Decode the URI */
	decoded = evhttp_uri_parse(uri);
	if (!decoded) {
		LOG_PRINT(LOG_INFO, "It's not a good URI. Sending BADREQUEST");
		evhttp_send_error(req, HTTP_BADREQUEST, 0);
		return;
	}

	/* Let's see what path the user asked for. */
	path = evhttp_uri_get_path(decoded);
	if (!path) path = "/";

    if(strstr(path, "favicon.ico"))
    {
        LOG_PRINT(LOG_INFO, "favicon.ico Request, Denied.");
        return;
    }
	LOG_PRINT(LOG_INFO, "Got a GET request for <%s>",  uri);


	/* We need to decode it, to see what path the user really wanted. */
	decoded_path = evhttp_uridecode(path, 0, NULL);
	if (decoded_path == NULL)
		goto err;
	/* Don't allow any ".."s in the path, to avoid exposing stuff outside
	 * of the docroot.  This test is both overzealous and underzealous:
	 * it forbids aceptable paths like "/this/one..here", but it doesn't
	 * do anything to prevent symlink following." */
	if (strstr(decoded_path, ".."))
		goto err;

	len = strlen(decoded_path)+strlen(_root_path)+1;
	if (!(whole_path = malloc(len))) {
		LOG_PRINT(LOG_ERROR, "malloc");
		goto err;
	}
	evutil_snprintf(whole_path, len, "%s%s", _root_path, decoded_path);

	/* This holds the content we're sending. */
	evb = evbuffer_new();

	if (stat(whole_path, &st)<0) {
        LOG_PRINT(LOG_WARNING, "Stat whole_path[%s] Failed! Goto zimg_cb() for Searching.", whole_path);
        char *md5 = (char *)malloc(strlen(decoded_path) + 1);
        if(decoded_path[0] == '/')
            strcpy(md5, decoded_path+1);
        char *path_end;
        if((path_end = strchr(md5, '/')) != 0)
            path_end[0] = '\0';
        int width, height, proportion, gray;
        struct evkeyvalq params;
        if(strchr(uri, '?') == 0)
        {
            width = 0;
            height = 0;
            proportion = 1;
            gray = 0;
        }
        else
        {
            evhttp_parse_query(uri, &params);
            if(evhttp_find_header(&params, "w"))
                width = atoi(evhttp_find_header(&params, "w"));
            else
                width = 0;
            if(evhttp_find_header(&params, "h"))
                height = atoi(evhttp_find_header(&params, "h"));
            else
                height = 0;
            if(evhttp_find_header(&params, "p"))
                proportion = atoi(evhttp_find_header(&params, "p"));
            else
                proportion = 1;
            if(evhttp_find_header(&params, "g"))
                gray = atoi(evhttp_find_header(&params, "g"));
            else
                gray = 0;
        }

        zimg_req = (zimg_req_t *)malloc(sizeof(zimg_req_t)); 
        zimg_req -> md5 = md5;
        zimg_req -> width = width;
        zimg_req -> height = height;
        zimg_req -> proportion = proportion;
        zimg_req -> gray = gray;
        //zimg_proportion = (proportion == 1 ? true : false);
        //zimg_gray = (gray == 1 ? true : false);

        char img_type[10];
		int get_img_rst = get_img(zimg_req, &buff, img_type, &len);


        if(get_img_rst == -1)
        {
            LOG_PRINT(LOG_ERROR, "zimg Requset Get Image[MD5: %s] Failed!", zimg_req->md5);
            goto err;
        }

        LOG_PRINT(LOG_INFO, "get buffer length: %d", len);
		evbuffer_add(evb, buff, len);

        LOG_PRINT(LOG_INFO, "Got the File!");
        LOG_PRINT(LOG_INFO, "img_type: %s", img_type);
		const char *type = guess_type(img_type);
        LOG_PRINT(LOG_INFO, "guess_img_type: %s", type);
		evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", type);
        evhttp_send_reply(req, 200, "OK", evb);


 		if(get_img_rst == 2)
		{
			if(new_img(buff, len, zimg_req->rspPath) == -1)
			{
				LOG_PRINT(LOG_WARNING, "New Image[%s] Save Failed!", zimg_req->rspPath);
			}
		}
		goto done;

	}


	if (S_ISDIR(st.st_mode)) {
		/* If it's a directory, read the comments and make a little
		 * index page */
		DIR *dir;
		struct dirent *ent;
		const char *trailing_slash = "";

		if (!strlen(path) || path[strlen(path)-1] != '/')
			trailing_slash = "/";

		if (!(dir = opendir(whole_path)))
			goto err;

		evbuffer_add_printf(evb, "<html>\n <head>\n"
		    "  <title>%s</title>\n"
		    "  <base href='%s%s%s'>\n"
		    " </head>\n"
		    " <body>\n"
		    "  <h1>%s</h1>\n"
		    "  <ul>\n",
		    decoded_path, /* XXX html-escape this. */
		    uri_root, path, /* XXX html-escape this? */
		    trailing_slash,
		    decoded_path /* XXX html-escape this */);
		while ((ent = readdir(dir))) {
			const char *name = ent->d_name;
			evbuffer_add_printf(evb,
			    "    <li><a href=\"%s\">%s</a>\n",
			    name, name);/* XXX escape this */
		}
		evbuffer_add_printf(evb, "</ul></body></html>\n");
		closedir(dir);
		evhttp_add_header(evhttp_request_get_output_headers(req),
		    "Content-Type", "text/html");
	} else {
		/* Otherwise it's a file; add it to the buffer to get
		 * sent via sendfile */
		const char *type = guess_content_type(decoded_path);
		if ((fd = open(whole_path, O_RDONLY)) < 0) 
        {
            LOG_PRINT(LOG_ERROR, "Open File[%s] Failed!", whole_path);
            goto err;
        }

		if (fstat(fd, &st)<0) {
			/* Make sure the length still matches, now that we
			 * opened the file :/ */
			LOG_PRINT(LOG_ERROR, "Fstat File[%s] Failed!", whole_path);
			goto err;
		}
		evhttp_add_header(evhttp_request_get_output_headers(req),
		    "Content-Type", type);
		evbuffer_add_file(evb, fd, 0, st.st_size);
	}

	evhttp_send_reply(req, 200, "OK", evb);
	goto done;
err:
	evhttp_send_error(req, 404, "404 Not Found!");
	if (fd>=0)
		close(fd);
done:
	if(buff)
		free(buff);
    if(zimg_req)
    {
        if(zimg_req->md5)
            free(zimg_req->md5);
        if(zimg_req->rspPath)
            free(zimg_req->rspPath);
        free(zimg_req);
    }
	if (decoded)
		evhttp_uri_free(decoded);
	if (decoded_path)
		free(decoded_path);
	if (whole_path)
		free(whole_path);
	if (evb)
		evbuffer_free(evb);
}

static int display_address(struct evhttp_bound_socket *handle)
{
    /* Extract and display the address we're listening on. */
    struct sockaddr_storage ss;
    evutil_socket_t fd;
    ev_socklen_t socklen = sizeof(ss);
    char addrbuf[128];
    void *inaddr;
    const char *addr;
    int got_port = -1;
    fd = evhttp_bound_socket_get_fd(handle);
    memset(&ss, 0, sizeof(ss));
    if (getsockname(fd, (struct sockaddr *)&ss, &socklen)) {
        LOG_PRINT(LOG_ERROR, "getsockname() failed");
        return -1;
    }
    if (ss.ss_family == AF_INET) {
        got_port = ntohs(((struct sockaddr_in*)&ss)->sin_port);
        inaddr = &((struct sockaddr_in*)&ss)->sin_addr;
    } else if (ss.ss_family == AF_INET6) {
        got_port = ntohs(((struct sockaddr_in6*)&ss)->sin6_port);
        inaddr = &((struct sockaddr_in6*)&ss)->sin6_addr;
    } else {
        fprintf(stderr, "Weird address family %d\n",
            ss.ss_family);
        return -1;
    }
    addr = evutil_inet_ntop(ss.ss_family, inaddr, addrbuf,
        sizeof(addrbuf));
    if (addr) {
        LOG_PRINT(LOG_INFO, "Listening on %s:%d", addr, got_port);
        evutil_snprintf(uri_root, sizeof(uri_root),
            "http://%s:%d",addr,got_port);
    } else {
        fprintf(stderr, "evutil_inet_ntop failed\n");
        return -1;
    }
    return 0;
}


int start_httpd(int port)
{
    struct event_base *base;
    struct evhttp *http;
    struct evhttp_bound_socket *handle;

    char *ip = "0.0.0.0";

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        return -1;
    base = event_base_new();
    if (!base) 
    {
        LOG_PRINT(LOG_ERROR, "Couldn't create an event_base: exiting");
        return -1;
    }

    /* Create a new evhttp object to handle requests. */
    http = evhttp_new(base);
    if (!http) 
    {
        LOG_PRINT(LOG_ERROR, "couldn't create evhttp. Exiting.");
        return -1;
    }

    /* The /dump URI will dump all requests to stdout and say 200 ok. */
    evhttp_set_cb(http, "/dump", dump_request_cb, NULL);
    evhttp_set_cb(http, "/upload", post_request_cb, NULL);

    /* We want to accept arbitrary requests, so we need to set a "generic"
     * cb.  We can also add callbacks for specific paths. */
    //evhttp_set_gencb(http, zimg_cb, _img_path);
    //evhttp_set_gencb(http, test_cb, NULL);
    evhttp_set_gencb(http, send_document_cb, NULL);

    /* Now we tell the evhttp what port to listen on */
    handle = evhttp_bind_socket_with_handle(http, ip, port);
    if (!handle) 
    {
        LOG_PRINT(LOG_ERROR, "couldn't bind to port %d. Exiting.", port);
        return -1;
    }

    display_address(handle);
    event_base_dispatch(base);
    return 0;
}
