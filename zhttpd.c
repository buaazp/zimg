#include "zhttpd.h"

// this data is for KMP searching 
int pi[128];
char uri_root[512];
extern char _img_path[];

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

	DEBUG_PRINT("Received a %s request for %s",
	    cmdtype, evhttp_request_get_uri(req));
    DEBUG_PRINT("Headers:");

	headers = evhttp_request_get_input_headers(req);
	for (header = headers->tqh_first; header;
	    header = header->next.tqe_next) {
		DEBUG_PRINT("  %s: %s", header->key, header->value);
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

/* KMP for searching */
void kmp_init(const unsigned char *pattern, int pattern_size)  // prefix-function
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

int kmp(const unsigned char *matcher, int mlen, const unsigned char *pattern, int plen)
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
        /*
        DEBUG_PRINT("i = %d, j = %d", i, j);
        if(matcher[i] == '\r')
            DEBUG_PRINT("buff[%d] = \\R", i);
        if(matcher[i+1] == '\n')
            DEBUG_PRINT("buff[%d] = \\N", i + 1);
            */
        return i;
    }
    else          // if no match was found
        return -1;
}

/* Callback used for the POST requset:
 * storage image data and gives back a trivial 200 ok */
static void post_request_cb(struct evhttp_request *req, void *arg)
{
    struct evbuffer *evb = NULL;
    struct evkeyvalq *headers;
    struct evkeyval *header;
    int postSize = 0;
    char *root_path = (char *)arg;
    char *boundary, *boundary_end = NULL;
    int boundary_len = 0;
    
	DEBUG_PRINT("Received a POST request for %s", evhttp_request_get_uri(req));
    DEBUG_PRINT("Headers:");

	headers = evhttp_request_get_input_headers(req);
	for (header = headers->tqh_first; header; header = header->next.tqe_next) 
    {
		DEBUG_PRINT("  %s: %s", header->key, header->value);
        if(strstr(header->key, "Content-Length") == header->key)
        {
            postSize = atoi(header->value);
        }
        else if(strstr(header->key, "Content-Type") == header->key)
        {
            if(strstr(header->value, "multipart/form-data") == 0)
            {
                DEBUG_ERROR("POST form error!");
                goto err;
            }
            else if(strstr(header->value, "boundary") == 0)
            {
                DEBUG_ERROR("boundary NOT found!");
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
                        DEBUG_ERROR("Invalid boundary in multipart/form-data POST data");
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

                DEBUG_PRINT("boundary Find. boundary = %s", boundary);
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
    int imgSize = 0, wlen = 0;
    int fd = -1;

    //************* print the binary data of img
    //puts("Input data: <<<");
    while((evblen = evbuffer_get_length(buf)) > 0)
    {
        DEBUG_PRINT("evblen = %d", evblen);
        rmblen = evbuffer_remove(buf, buff, evblen);
        DEBUG_PRINT("rmblen = %d", rmblen);
        if(rmblen < 0)
        {
            DEBUG_ERROR("evbuffer_remove failed!");
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
    DEBUG_PRINT("input data>>>");
    (void) fwrite(buff, 1, 2048, stdout);
    printf("\n");
    DEBUG_PRINT(">>>");
    */

    int start = -1, end = -1;
    //char fileNamePattern[] = {'f', 'i', 'l', 'e', 'n', 'a', 'm', 'e', '='};
    //char typePattern[] = {'C', 'o', 'n', 't', 'e', 'n', 't', '-', 'T', 'y', 'p', 'e'};
    char *fileNamePattern = "filename=";
    char *typePattern = "Content-Type";
    char *quotePattern = "\"";
    char *blankPattern = "\r\n";
    //char quotePattern[] = {'\"'};
    //char blankPattern[] = {'\r', '\n'};
    char *boundaryPattern = (char *)malloc(boundary_len + 4);
    sprintf(boundaryPattern, "\r\n--%s", boundary);
    /*
    DEBUG_PRINT("boundaryPattern[%d] = %c", boundary_len + 3, boundaryPattern[boundary_len+3]);
    if(boundaryPattern[boundary_len+4] == '\0')
        DEBUG_PRINT("boundaryPattern[%d] = %s", boundary_len + 4, "\\0");
    else
        DEBUG_PRINT("boundaryPattern[%d] = %c", boundary_len + 4, boundaryPattern[boundary_len+4]);
    */
    DEBUG_PRINT("boundaryPattern = %s, strlen = %d", boundaryPattern, (int)strlen(boundaryPattern));
    if((start = kmp(buff, postSize, fileNamePattern, strlen(fileNamePattern))) == -1)
    {
        DEBUG_ERROR("Content-Disposition Not Found!");
        goto err;
    }
    start += 9;
    if(buff[start] == '\"')
    {
        start++;
        if((end = kmp(buff+start, postSize-start, quotePattern, strlen(quotePattern))) == -1)
        {
            DEBUG_ERROR("quote \" Not Found!");
            goto err;
        }
    }
    else
    {
        if((end = kmp(buff+start, postSize-start, blankPattern, strlen(blankPattern))) == -1)
        {
            DEBUG_ERROR("quote \" Not Found!");
            goto err;
        }
    }
    fileName = (char *)malloc(end + 1);
    memcpy(fileName, buff+start, end);
    fileName[end] = '\0';
    DEBUG_PRINT("fileName = %s", fileName);

    char fileType[64];
    if(getType(fileName, fileType) == -1)
    {
        DEBUG_ERROR("Get Type of File[%s] Failed!", fileName);
        goto err;
    }
    if(isImg(fileType) != 1)
    {
        DEBUG_ERROR("fileType[%s] is Not Supported!", fileType);
        goto err;
    }

    end += start;

    if((start = kmp(buff+end, postSize-end, typePattern, strlen(typePattern))) == -1)
    {
        DEBUG_ERROR("Content-Type Not Found!");
        goto err;
    }
    start += end;
    DEBUG_PRINT("start = %d", start);
    if((end =  kmp(buff+start, postSize-start, blankPattern, strlen(blankPattern))) == -1)
    {
        DEBUG_ERROR("Image Not complete!");
        goto err;
    }
    end += start;
    DEBUG_PRINT("end = %d", end);
    start = end + 4;
    DEBUG_PRINT("start = %d", start);
    if((end = kmp(buff+start, postSize-start, boundaryPattern, strlen(boundaryPattern))) == -1)
    {
        DEBUG_ERROR("Image Not complete!");
        goto err;
    }
    end += start;
    DEBUG_PRINT("end = %d", end);
    imgSize = end - start;


    DEBUG_PRINT("postSize = %d", postSize);
    DEBUG_PRINT("imgSize = %d", imgSize);

    DEBUG_PRINT("Begin to Caculate MD5...");
    unsigned char md[16];
    int i;
    char tmp[3]={'\0'}, md5sum[33]={'\0'};
    MD5(buff+start, imgSize, md);
    for (i = 0; i < 16; i++)
    {
        sprintf(tmp, "%2.2x", md[i]);
        strcat(md5sum, tmp);
    }
    DEBUG_PRINT("[%s] md5: %s", fileName, md5sum);

    char *savePath = (char *)malloc(512);
    char *saveName= (char *)malloc(512);
    char *origName = (char *)malloc(512);
    sprintf(savePath, "%s/%s", _img_path, md5sum);
    DEBUG_PRINT("savePath: %s", savePath);
    if(isDir(savePath) == -1)
    {
        if(mkDir(savePath) == -1)
        {
            DEBUG_ERROR("savePath[%s] Create Failed!", savePath);
            goto err;
        }
    }
    /*
    if(access(savePath, 0) == -1)
    {
        DEBUG_PRINT("Begin to mkdir...");
        int status = mkdir(savePath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if(status == -1)
        {
            DEBUG_ERROR("MKDIR Failed!");
            goto err;
        }
        DEBUG_PRINT("Mkdir sucessfully!");
    }
    */
    sprintf(origName, "0rig.%s", fileType);
    sprintf(saveName, "%s/%s", savePath, origName);
    DEBUG_PRINT("saveName-->: %s", saveName);

    /*
    char *savePath = (char *)malloc(512);
    char *saveName = (char *)malloc(512);
    strcpy(savePath, _img_path);
    int svplen = strlen(savePath);
    if(savePath[svplen - 1] != '/')
    {
        savePath[svplen] = '/';
        savePath[svplen + 1] != '\0';
    }
    sprintf(saveName, "%s%s", savePath, fileName);
    DEBUG_PRINT("saveName: %s", saveName);
    */

    if((fd = open(saveName, O_WRONLY|O_TRUNC|O_CREAT, 00644)) < 0)
    {
        DEBUG_ERROR("fd open failed!");
        goto err;
    }
    wlen = write(fd, buff+start, imgSize);
    if(wlen == -1)
    {
        DEBUG_ERROR("write() failed!");
        close(fd);
        goto err;
    }
    else if(wlen < imgSize)
    {
        DEBUG_ERROR("Only part of data is been writed.");
        close(fd);
        goto err;
    }
    if(fd != -1)
    {
        close(fd);
    }
    DEBUG_PRINT("Image [%s] Write Successfully!", saveName);

    /*
    if(rename(saveName, newName) < 0)
    {
        DEBUG_ERROR("Rename Failed!");
        goto err;
    }
    */

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
    if (evb)
        evbuffer_free(evb);
    DEBUG_PRINT("============post_request_cb() DONE!===============");
    return;

err:
    evhttp_send_error(req, 500, "Image Upload Failed!");
    DEBUG_PRINT("============post_request_cb() ERROR!===============");
}

/* Callback used for zimg servise, such as:
 * http://127.0.0.1:8080/c6c4949e54afdb0972d323028657a1ef?w=100&h=50&scaled=1 */
static void zimg_cb(struct evhttp_request *req, void *arg)
{
    struct evbuffer *evb = NULL;
    const char *docroot = arg;
    const char *uri = evhttp_request_get_uri(req);
    struct evhttp_uri *decoded = NULL;
    struct evkeyvalq params;
    const char *path;
    char *decoded_path;
    char *whole_path = NULL;
    size_t len;
    int fd = -1;
    struct stat st;
    int width, height, scaled;

    /*
    if (evhttp_request_get_command(req) == EVHTTP_REQ_POST) 
    {
        post_request_cb(req, arg);
        return;
    }
    else if(evhttp_request_get_command(req) != EVHTTP_REQ_GET){
        dump_request_cb(req, arg);
        return;
    }
    */

    /* Decode the URI */
    decoded = evhttp_uri_parse(uri);
    if (!decoded) {
        DEBUG_PRINT("It's not a good URI. Sending BADREQUEST");
        evhttp_send_error(req, HTTP_BADREQUEST, 0);
        return;
    }

    /* Let's see what path the user asked for. */
    path = evhttp_uri_get_path(decoded);
    if (!path) path = "/";

    if(strstr(path, "favicon.ico"))
    {
        DEBUG_PRINT("favicon.ico Request, Denied.");
        return;
    }
    DEBUG_PRINT("Got a zimg request for <%s>",  uri);

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

    len = strlen(decoded_path)+strlen(docroot)+1;
    if (!(whole_path = malloc(len))) {
        DEBUG_ERROR("malloc failed!");
        goto err;
    }
    evutil_snprintf(whole_path, len, "%s%s", docroot, decoded_path);

    if (stat(whole_path, &st)<0) {
        DEBUG_ERROR("stat whole_path[%s] failed!", whole_path);
        goto err;
    }

    /* This holds the content we're sending. */
    evb = evbuffer_new();

    if (S_ISDIR(st.st_mode)) 
    {
        /* If it's a directory, read the comments and make a little
         * index page */
        char *rspPath;
        char *origPath;

        if(strchr(uri, '?') == 0)
        {
            width = 0;
            height = 0;
            scaled = 1;
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
            if(evhttp_find_header(&params, "scaled"))
                scaled = atoi(evhttp_find_header(&params, "scaled"));
            else
                scaled = 1;
        }

        /* test time used
        struct timeval tv;
        gettimeofday(&tv);
        printf("tv.sec: %d\n", tv.tv_sec);
        printf("tv.usec: %d\n", tv.tv_usec);
        */

        DIR *dir;
        if (!(dir = opendir(whole_path)))
        {
            DEBUG_ERROR("Dir[%s] open failed.", whole_path);
            goto err;
        }
        char name[32];
        sprintf(name, "%d*%d", width, height);
        struct dirent *ent;
        char *origName;
        char *imgType;
        int find = 0;

        while(ent = readdir(dir))
        {
            const char *tmpName = ent->d_name;
            if(strstr(tmpName, "0rig") == tmpName) // name "0rig" to find it first
            {
                len = strlen(tmpName) + 1;
                if(!(origName = malloc(len)))
                {
                    DEBUG_ERROR("malloc");
                    goto err;
                }
                strcpy(origName, tmpName);
                imgType = strchr(origName, '.');
                imgType++;
                find = 1;
                break;
            }
        }
        closedir(dir);
        if(find == 0)
        {
            DEBUG_ERROR("Get 0rig Image Failed.");
            goto err;
        }
        else
        {
            len = strlen(whole_path) + strlen(origName) + 2;
            if(!(origPath = malloc(len)))
            {
                DEBUG_ERROR("malloc");
                goto err;
            }
            sprintf(origPath, "%s/%s", whole_path, origName);
            DEBUG_PRINT("0rig File Path: %s", origPath);
            if(width == 0)
            {
                DEBUG_PRINT("Return original image.");
                if(!(rspPath = malloc(len)))
                {
                    DEBUG_ERROR("malloc");
                    goto err;
                }
                strcpy(rspPath, origPath);
            }
            else
            {
                // rspPath = whole_path / 1024*768 . jpeg \0
                len = strlen(whole_path) + strlen(name) + strlen(imgType) + 3; 
                if(!(rspPath = malloc(len)))
                {
                    DEBUG_ERROR("malloc");
                    goto err;
                }
                sprintf(rspPath, "%s/%s.%s", whole_path, name, imgType);
            }
            DEBUG_PRINT("File Path: %s", rspPath);
        }
        /* Otherwise it's a file; add it to the buffer to get
         * sent via sendfile */
        const char *type = guess_content_type(origName);
        if ((fd = open(rspPath, O_RDONLY)) < 0) 
        {
            DEBUG_PRINT("Not find the file, begin to resize...");
            MagickBooleanType status;
            MagickWand *magick_wand;
            MagickWandGenesis();
            magick_wand=NewMagickWand();
            status=MagickReadImage(magick_wand, origPath);
            if (status == MagickFalse)
            {
                ThrowWandException(magick_wand);
                goto err;
            }
            if(scaled == 1)
            {
                int owidth = MagickGetImageWidth(magick_wand);
                float oheight = MagickGetImageHeight(magick_wand);
                height = width * oheight / owidth;
            }
            MagickResetIterator(magick_wand);
            while (MagickNextImage(magick_wand) != MagickFalse)
                MagickResizeImage(magick_wand, width, height, LanczosFilter, 1.0);
            DEBUG_PRINT("Resize img succ.");
            MagickSizeType imgSize;
            status = MagickGetImageLength(magick_wand, &imgSize);
            if (status == MagickFalse)
            {
                ThrowWandException(magick_wand);
                goto err;
            }
            size_t imgSizet = imgSize;
            char *imgBuff = MagickGetImageBlob(magick_wand, &imgSizet);
            evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", type);
            //Designed for High Performance: Reply Customer First, Storage Image Then.
            evbuffer_add(evb, imgBuff, imgSizet);
            evhttp_send_reply(req, 200, "OK", evb);
            status=MagickWriteImages(magick_wand, rspPath, MagickTrue);
            if (status == MagickFalse)
            {
                ThrowWandException(magick_wand);
                DEBUG_WARNING("New img[%s] Write Failed!", rspPath);
            }
            else
            {
                DEBUG_PRINT("New img[%s] storaged.", rspPath);
            }
            magick_wand=DestroyMagickWand(magick_wand);
            MagickWandTerminus();
            goto done;
        }
        else if (fstat(fd, &st)<0) 
        {
            /* Make sure the length still matches, now that we
             * opened the file :/ */
            DEBUG_ERROR("fstat failed.");
            goto err;
        }
        else
        {
            DEBUG_PRINT("Got the file!");
            evbuffer_add_file(evb, fd, 0, st.st_size);
        }
        evhttp_add_header(evhttp_request_get_output_headers(req), "Content-Type", type);
        evhttp_send_reply(req, 200, "OK", evb);
        goto done;
    }
    else 
    {
        DEBUG_ERROR("MD5[%s] not find.", decoded_path);
        goto err;
    }

err:
    evhttp_send_error(req, 404, "Image was not found");
    if (fd>=0)
        close(fd);

done:
    if (decoded)
        evhttp_uri_free(decoded);
    if (decoded_path)
        free(decoded_path);
    if (whole_path)
        free(whole_path);
    if (evb)
        evbuffer_free(evb);

    return;
}

/* This callback gets invoked when we get any http request that doesn't match
 * any other callback.  Like any evhttp server callback, it has a simple job:
 * it must eventually call evhttp_send_error() or evhttp_send_reply().
 */
static void send_document_cb(struct evhttp_request *req, void *arg)
{
	struct evbuffer *evb = NULL;
	const char *docroot = arg;
	const char *uri = evhttp_request_get_uri(req);
	struct evhttp_uri *decoded = NULL;
	const char *path;
	char *decoded_path;
	char *whole_path = NULL;
	size_t len;
	int fd = -1;
	struct stat st;

    if (evhttp_request_get_command(req) == EVHTTP_REQ_POST) 
    {
        DEBUG_PRINT("POST --> post_request_cb(req, %s)", _img_path);
        post_request_cb(req, _img_path);
        /*
        if(post_request_cb(req, _img_path) == -1)
        {
            DEBUG_ERROR("post_request_cb() call failed!");
            return;
        }
        */
        DEBUG_PRINT("post_request_cb(req, %s) --> POST", _img_path);
        return;
    }
    else if(evhttp_request_get_command(req) != EVHTTP_REQ_GET){
        dump_request_cb(req, arg);
        return;
    }

	/* Decode the URI */
	decoded = evhttp_uri_parse(uri);
	if (!decoded) {
		DEBUG_PRINT("It's not a good URI. Sending BADREQUEST");
		evhttp_send_error(req, HTTP_BADREQUEST, 0);
		return;
	}

	/* Let's see what path the user asked for. */
	path = evhttp_uri_get_path(decoded);
	if (!path) path = "/";

    if(strstr(path, "favicon.ico"))
    {
        DEBUG_PRINT("favicon.ico Request, Denied.");
        return;
    }
	DEBUG_PRINT("Got a GET request for <%s>",  uri);


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

	len = strlen(decoded_path)+strlen(docroot)+1;
	if (!(whole_path = malloc(len))) {
		DEBUG_ERROR("malloc");
		goto err;
	}
	evutil_snprintf(whole_path, len, "%s%s", docroot, decoded_path);

	if (stat(whole_path, &st)<0) {
        DEBUG_WARNING("Stat whole_path[%s] Failed! Goto zimg_cb() for Searching.", whole_path);
        zimg_cb(req, _img_path);
        /*
        if(zimg_cb(req, _img_path) == -1)
        {
            DEBUG_ERROR("zimg_cb call failed!");
        }
        */
        goto done;
	}

	/* This holds the content we're sending. */
	evb = evbuffer_new();

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
            DEBUG_ERROR("Open File[%s] Failed!", whole_path);
            goto err;
        }

		if (fstat(fd, &st)<0) {
			/* Make sure the length still matches, now that we
			 * opened the file :/ */
			DEBUG_ERROR("Fstat File[%s] Failed!", whole_path);
			goto err;
		}
		evhttp_add_header(evhttp_request_get_output_headers(req),
		    "Content-Type", type);
		evbuffer_add_file(evb, fd, 0, st.st_size);
	}

	evhttp_send_reply(req, 200, "OK", evb);
	goto done;
err:
	evhttp_send_error(req, 404, "Page was not found!");
	if (fd>=0)
		close(fd);
done:
	if (decoded)
		evhttp_uri_free(decoded);
	if (decoded_path)
		free(decoded_path);
	if (whole_path)
		free(whole_path);
	if (evb)
		evbuffer_free(evb);
}

static int displayAddress(struct evhttp_bound_socket *handle)
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
        DEBUG_ERROR("getsockname() failed");
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
        DEBUG_PRINT("Listening on %s:%d", addr, got_port);
        evutil_snprintf(uri_root, sizeof(uri_root),
            "http://%s:%d",addr,got_port);
    } else {
        fprintf(stderr, "evutil_inet_ntop failed\n");
        return -1;
    }
    return 0;
}

int startHttpd(int port, char *root_path)
{
    struct event_base *base;
    struct evhttp *http;
    struct evhttp_bound_socket *handle;

    char *ip = "127.0.0.1";

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        return -1;
    base = event_base_new();
    if (!base) 
    {
        DEBUG_ERROR("Couldn't create an event_base: exiting");
        return -1;
    }

    /* Create a new evhttp object to handle requests. */
    http = evhttp_new(base);
    if (!http) 
    {
        DEBUG_ERROR("couldn't create evhttp. Exiting.");
        return -1;
    }

    /* The /dump URI will dump all requests to stdout and say 200 ok. */
    evhttp_set_cb(http, "/dump", dump_request_cb, NULL);
    evhttp_set_cb(http, "/upload", post_request_cb, _img_path);

    /* We want to accept arbitrary requests, so we need to set a "generic"
     * cb.  We can also add callbacks for specific paths. */
    //evhttp_set_gencb(http, zimg_cb, _img_path);
    //evhttp_set_gencb(http, post_request_cb, _img_path);
    evhttp_set_gencb(http, send_document_cb, root_path);

    /* Now we tell the evhttp what port to listen on */
    handle = evhttp_bind_socket_with_handle(http, ip, port);
    if (!handle) 
    {
        DEBUG_ERROR("couldn't bind to port %d. Exiting.", port);
        return -1;
    }

    displayAddress(handle);
    event_base_dispatch(base);
    return 0;
}
