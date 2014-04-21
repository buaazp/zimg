#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <signal.h>
#include <strings.h>
#include <inttypes.h>
#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#else
#define WINVER 0x0501
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#ifndef NO_SYS_UN
#include <sys/un.h>
#endif

//#define EVHTP_DISABLE_SSL 1

#include <limits.h>

#include "evhtp.h"

static int                  _evhtp_request_parser_start(htparser * p);
static int                  _evhtp_request_parser_path(htparser * p, const char * data, size_t len);
static int                  _evhtp_request_parser_args(htparser * p, const char * data, size_t len);
static int                  _evhtp_request_parser_header_key(htparser * p, const char * data, size_t len);
static int                  _evhtp_request_parser_header_val(htparser * p, const char * data, size_t len);
static int                  _evhtp_request_parser_hostname(htparser * p, const char * data, size_t len);
static int                  _evhtp_request_parser_headers(htparser * p);
static int                  _evhtp_request_parser_body(htparser * p, const char * data, size_t len);
static int                  _evhtp_request_parser_fini(htparser * p);
static int                  _evhtp_request_parser_chunk_new(htparser * p);
static int                  _evhtp_request_parser_chunk_fini(htparser * p);
static int                  _evhtp_request_parser_chunks_fini(htparser * p);
static int                  _evhtp_request_parser_headers_start(htparser * p);

static void                 _evhtp_connection_readcb(evbev_t * bev, void * arg);

static evhtp_connection_t * _evhtp_connection_new(evhtp_t * htp, evutil_socket_t sock, evhtp_type type);

static evhtp_uri_t        * _evhtp_uri_new(void);
static void                 _evhtp_uri_free(evhtp_uri_t * uri);

static evhtp_path_t       * _evhtp_path_new(const char * data, size_t len);
static void                 _evhtp_path_free(evhtp_path_t * path);

#define HOOK_AVAIL(var, hook_name)                 (var->hooks && var->hooks->hook_name)
#define HOOK_FUNC(var, hook_name)                  (var->hooks->hook_name)
#define HOOK_ARGS(var, hook_name)                  var->hooks->hook_name ## _arg

#define HOOK_REQUEST_RUN(request, hook_name, ...)  do {                                       \
        if (HOOK_AVAIL(request, hook_name)) {                                                 \
            return HOOK_FUNC(request, hook_name) (request, __VA_ARGS__,                       \
                                                  HOOK_ARGS(request, hook_name));             \
        }                                                                                     \
                                                                                              \
        if (HOOK_AVAIL(evhtp_request_get_connection(request), hook_name)) {                   \
            return HOOK_FUNC(request->conn, hook_name) (request, __VA_ARGS__,                 \
                                                        HOOK_ARGS(request->conn, hook_name)); \
        }                                                                                     \
} while (0)

#define HOOK_REQUEST_RUN_NARGS(request, hook_name) do {                                       \
        if (HOOK_AVAIL(request, hook_name)) {                                                 \
            return HOOK_FUNC(request, hook_name) (request,                                    \
                                                  HOOK_ARGS(request, hook_name));             \
        }                                                                                     \
                                                                                              \
        if (HOOK_AVAIL(request->conn, hook_name)) {                                           \
            return HOOK_FUNC(request->conn, hook_name) (request,                              \
                                                        HOOK_ARGS(request->conn, hook_name)); \
        }                                                                                     \
} while (0);

#ifndef EVHTP_DISABLE_EVTHR
#define _evhtp_lock(h)                             do { \
        if (h->lock) {                                  \
            pthread_mutex_lock(h->lock);                \
        }                                               \
} while (0)

#define _evhtp_unlock(h)                           do { \
        if (h->lock) {                                  \
            pthread_mutex_unlock(h->lock);              \
        }                                               \
} while (0)
#else
#define _evhtp_lock(h)                             do {} while (0)
#define _evhtp_unlock(h)                           do {} while (0)
#endif

#ifndef TAILQ_FOREACH_SAFE
#define TAILQ_FOREACH_SAFE(var, head, field, tvar)        \
    for ((var) = TAILQ_FIRST((head));                     \
         (var) && ((tvar) = TAILQ_NEXT((var), field), 1); \
         (var) = (tvar))
#endif

const char *
status_code_to_str(evhtp_res code) {
    switch (code) {
        case EVHTP_RES_200:
            return "OK";
        case EVHTP_RES_300:
            return "Redirect";
        case EVHTP_RES_400:
            return "Bad Request";
        case EVHTP_RES_NOTFOUND:
            return "Not Found";
        case EVHTP_RES_SERVERR:
            return "Internal Server Error";
        case EVHTP_RES_CONTINUE:
            return "Continue";
        case EVHTP_RES_FORBIDDEN:
            return "Forbidden";
        case EVHTP_RES_SWITCH_PROTO:
            return "Switching Protocols";
        case EVHTP_RES_MOVEDPERM:
            return "Moved Permanently";
        case EVHTP_RES_PROCESSING:
            return "Processing";
        case EVHTP_RES_URI_TOOLONG:
            return "URI Too Long";
        case EVHTP_RES_CREATED:
            return "Created";
        case EVHTP_RES_ACCEPTED:
            return "Accepted";
        case EVHTP_RES_NAUTHINFO:
            return "No Auth Info";
        case EVHTP_RES_NOCONTENT:
            return "No Content";
        case EVHTP_RES_RSTCONTENT:
            return "Reset Content";
        case EVHTP_RES_PARTIAL:
            return "Partial Content";
        case EVHTP_RES_MSTATUS:
            return "Multi-Status";
        case EVHTP_RES_IMUSED:
            return "IM Used";
        case EVHTP_RES_FOUND:
            return "Found";
        case EVHTP_RES_SEEOTHER:
            return "See Other";
        case EVHTP_RES_NOTMOD:
            return "Not Modified";
        case EVHTP_RES_USEPROXY:
            return "Use Proxy";
        case EVHTP_RES_SWITCHPROXY:
            return "Switch Proxy";
        case EVHTP_RES_TMPREDIR:
            return "Temporary Redirect";
        case EVHTP_RES_UNAUTH:
            return "Unauthorized";
        case EVHTP_RES_PAYREQ:
            return "Payment Required";
        case EVHTP_RES_METHNALLOWED:
            return "Not Allowed";
        case EVHTP_RES_NACCEPTABLE:
            return "Not Acceptable";
        case EVHTP_RES_PROXYAUTHREQ:
            return "Proxy Authentication Required";
        case EVHTP_RES_TIMEOUT:
            return "Request Timeout";
        case EVHTP_RES_CONFLICT:
            return "Conflict";
        case EVHTP_RES_GONE:
            return "Gone";
        case EVHTP_RES_LENREQ:
            return "Length Required";
        case EVHTP_RES_PRECONDFAIL:
            return "Precondition Failed";
        case EVHTP_RES_ENTOOLARGE:
            return "Entity Too Large";
        case EVHTP_RES_URITOOLARGE:
            return "Request-URI Too Long";
        case EVHTP_RES_UNSUPPORTED:
            return "Unsupported Media Type";
        case EVHTP_RES_RANGENOTSC:
            return "Requested Range Not Satisfiable";
        case EVHTP_RES_EXPECTFAIL:
            return "Expectation Failed";
        case EVHTP_RES_IAMATEAPOT:
            return "I'm a teapot";
        case EVHTP_RES_NOTIMPL:
            return "Not Implemented";
        case EVHTP_RES_BADGATEWAY:
            return "Bad Gateway";
        case EVHTP_RES_SERVUNAVAIL:
            return "Service Unavailable";
        case EVHTP_RES_GWTIMEOUT:
            return "Gateway Timeout";
        case EVHTP_RES_VERNSUPPORT:
            return "HTTP Version Not Supported";
        case EVHTP_RES_BWEXEED:
            return "Bandwidth Limit Exceeded";
    } /* switch */

    return "UNKNOWN";
}     /* status_code_to_str */

/**
 * @brief callback definitions for request processing from libhtparse
 */
static htparse_hooks request_psets = {
    .on_msg_begin       = _evhtp_request_parser_start,
    .method             = NULL,
    .scheme             = NULL,
    .host               = NULL,
    .port               = NULL,
    .path               = _evhtp_request_parser_path,
    .args               = _evhtp_request_parser_args,
    .uri                = NULL,
    .on_hdrs_begin      = _evhtp_request_parser_headers_start,
    .hdr_key            = _evhtp_request_parser_header_key,
    .hdr_val            = _evhtp_request_parser_header_val,
    .hostname           = _evhtp_request_parser_hostname,
    .on_hdrs_complete   = _evhtp_request_parser_headers,
    .on_new_chunk       = _evhtp_request_parser_chunk_new,
    .on_chunk_complete  = _evhtp_request_parser_chunk_fini,
    .on_chunks_complete = _evhtp_request_parser_chunks_fini,
    .body               = _evhtp_request_parser_body,
    .on_msg_complete    = _evhtp_request_parser_fini
};

#ifndef EVHTP_DISABLE_SSL
static int             session_id_context    = 1;
#ifndef EVHTP_DISABLE_EVTHR
static int             ssl_num_locks;
static evhtp_mutex_t * ssl_locks;
static int             ssl_locks_initialized = 0;
#endif
#endif

/*
 * COMPAT FUNCTIONS
 */

#ifdef NO_STRNLEN
static size_t
strnlen(const char * s, size_t maxlen) {
    const char * e;
    size_t       n;

    for (e = s, n = 0; *e && n < maxlen; e++, n++) {
        ;
    }

    return n;
}

#endif

#ifdef NO_STRNDUP
static char *
strndup(const char * s, size_t n) {
    size_t len = strnlen(s, n);
    char * ret;

    if (len < n) {
        return strdup(s);
    }

    ret    = malloc(n + 1);
    ret[n] = '\0';

    strncpy(ret, s, n);
    return ret;
}

#endif

/*
 * PRIVATE FUNCTIONS
 */

/**
 * @brief a weak hash function
 *
 * @param str a null terminated string
 *
 * @return an unsigned integer hash of str
 */
static inline unsigned int
_evhtp_quick_hash(const char * str) {
    unsigned int h = 0;

    for (; *str; str++) {
        h = 31 * h + *str;
    }

    return h;
}

/**
 * @brief helper function to determine if http version is HTTP/1.0
 *
 * @param major the major version number
 * @param minor the minor version number
 *
 * @return 1 if HTTP/1.0, else 0
 */
static inline int
_evhtp_is_http_10(const char major, const char minor) {
    if (major >= 1 && minor <= 0) {
        return 1;
    }

    return 0;
}

/**
 * @brief helper function to determine if http version is HTTP/1.1
 *
 * @param major the major version number
 * @param minor the minor version number
 *
 * @return 1 if HTTP/1.1, else 0
 */
static inline int
_evhtp_is_http_11(const char major, const char minor) {
    if (major >= 1 && minor >= 1) {
        return 1;
    }

    return 0;
}

/**
 * @brief returns the HTTP protocol version
 *
 * @param major the major version number
 * @param minor the minor version number
 *
 * @return EVHTP_PROTO_10 if HTTP/1.0, EVHTP_PROTO_11 if HTTP/1.1, otherwise
 *         EVHTP_PROTO_INVALID
 */
static inline evhtp_proto
_evhtp_protocol(const char major, const char minor) {
    if (_evhtp_is_http_10(major, minor)) {
        return EVHTP_PROTO_10;
    }

    if (_evhtp_is_http_11(major, minor)) {
        return EVHTP_PROTO_11;
    }

    return EVHTP_PROTO_INVALID;
}

/**
 * @brief runs the user-defined on_path hook for a request
 *
 * @param request the request structure
 * @param path the path structure
 *
 * @return EVHTP_RES_OK on success, otherwise something else.
 */
static inline evhtp_res
_evhtp_path_hook(evhtp_request_t * request, evhtp_path_t * path) {
    HOOK_REQUEST_RUN(request, on_path, path);

    return EVHTP_RES_OK;
}

/**
 * @brief runs the user-defined on_header hook for a request
 *
 * once a full key: value header has been parsed, this will call the hook
 *
 * @param request the request strucutre
 * @param header the header structure
 *
 * @return EVHTP_RES_OK on success, otherwise something else.
 */
static inline evhtp_res
_evhtp_header_hook(evhtp_request_t * request, evhtp_header_t * header) {
    HOOK_REQUEST_RUN(request, on_header, header);

    return EVHTP_RES_OK;
}

/**
 * @brief runs the user-defined on_Headers hook for a request after all headers
 *        have been parsed.
 *
 * @param request the request structure
 * @param headers the headers tailq structure
 *
 * @return EVHTP_RES_OK on success, otherwise something else.
 */
static inline evhtp_res
_evhtp_headers_hook(evhtp_request_t * request, evhtp_headers_t * headers) {
    HOOK_REQUEST_RUN(request, on_headers, headers);

    return EVHTP_RES_OK;
}

/**
 * @brief runs the user-defined on_body hook for requests containing a body.
 *        the data is stored in the request->buffer_in so the user may either
 *        leave it, or drain upon being called.
 *
 * @param request the request strucutre
 * @param buf a evbuffer containing body data
 *
 * @return EVHTP_RES_OK on success, otherwise something else.
 */
static inline evhtp_res
_evhtp_body_hook(evhtp_request_t * request, evbuf_t * buf) {
    HOOK_REQUEST_RUN(request, on_read, buf);

    return EVHTP_RES_OK;
}

/**
 * @brief runs the user-defined hook called just prior to a request been
 *        free()'d
 *
 * @param request therequest structure
 *
 * @return EVHTP_RES_OK on success, otherwise treated as an error
 */
static inline evhtp_res
_evhtp_request_fini_hook(evhtp_request_t * request) {
    HOOK_REQUEST_RUN_NARGS(request, on_request_fini);

    return EVHTP_RES_OK;
}

static inline evhtp_res
_evhtp_chunk_new_hook(evhtp_request_t * request, uint64_t len) {
    HOOK_REQUEST_RUN(request, on_new_chunk, len);

    return EVHTP_RES_OK;
}

static inline evhtp_res
_evhtp_chunk_fini_hook(evhtp_request_t * request) {
    HOOK_REQUEST_RUN_NARGS(request, on_chunk_fini);

    return EVHTP_RES_OK;
}

static inline evhtp_res
_evhtp_chunks_fini_hook(evhtp_request_t * request) {
    HOOK_REQUEST_RUN_NARGS(request, on_chunks_fini);

    return EVHTP_RES_OK;
}

static inline evhtp_res
_evhtp_headers_start_hook(evhtp_request_t * request) {
    HOOK_REQUEST_RUN_NARGS(request, on_headers_start);

    return EVHTP_RES_OK;
}

/**
 * @brief runs the user-definedhook called just prior to a connection being
 *        closed
 *
 * @param connection the connection structure
 *
 * @return EVHTP_RES_OK on success, but pretty much ignored in any case.
 */
static inline evhtp_res
_evhtp_connection_fini_hook(evhtp_connection_t * connection) {
    if (connection->hooks && connection->hooks->on_connection_fini) {
        return (connection->hooks->on_connection_fini)(connection,
                                                       connection->hooks->on_connection_fini_arg);
    }

    return EVHTP_RES_OK;
}

static inline evhtp_res
_evhtp_hostname_hook(evhtp_request_t * r, const char * hostname) {
    HOOK_REQUEST_RUN(r, on_hostname, hostname);

    return EVHTP_RES_OK;
}

static inline evhtp_res
_evhtp_connection_write_hook(evhtp_connection_t * connection) {
    if (connection->hooks && connection->hooks->on_write) {
        return (connection->hooks->on_write)(connection,
                                             connection->hooks->on_write_arg);
    }

    return EVHTP_RES_OK;
}

/**
 * @brief glob/wildcard type pattern matching.
 *
 * Note: This code was derived from redis's (v2.6) stringmatchlen() function.
 *
 * @param pattern
 * @param string
 *
 * @return
 */
static int
_evhtp_glob_match(const char * pattern, const char * string) {
    size_t pat_len;
    size_t str_len;

    if (!pattern || !string) {
        return 0;
    }

    pat_len = strlen(pattern);
    str_len = strlen(string);

    while (pat_len) {
        if (pattern[0] == '*') {
            while (pattern[1] == '*') {
                pattern++;
                pat_len--;
            }

            if (pat_len == 1) {
                return 1;
            }

            while (str_len) {
                if (_evhtp_glob_match(pattern + 1, string)) {
                    return 1;
                }

                string++;
                str_len--;
            }

            return 0;
        } else {
            if (pattern[0] != string[0]) {
                return 0;
            }

            string++;
            str_len--;
        }

        pattern++;
        pat_len--;

        if (str_len == 0) {
            while (*pattern == '*') {
                pattern++;
                pat_len--;
            }
            break;
        }
    }

    if (pat_len == 0 && str_len == 0) {
        return 1;
    }

    return 0;
} /* _evhtp_glob_match */

static evhtp_callback_t *
_evhtp_callback_find(evhtp_callbacks_t * cbs,
                     const char        * path,
                     unsigned int      * start_offset,
                     unsigned int      * end_offset) {
#ifndef EVHTP_DISABLE_REGEX
    regmatch_t         pmatch[28];
#endif
    evhtp_callback_t * callback;

    if (cbs == NULL) {
        return NULL;
    }

    TAILQ_FOREACH(callback, cbs, next) {
        switch (callback->type) {
            case evhtp_callback_type_hash:
                if (strcmp(callback->val.path, path) == 0) {
                    *start_offset = 0;
                    *end_offset   = (unsigned int)strlen(path);
                    return callback;
                }
                break;
#ifndef EVHTP_DISABLE_REGEX
            case evhtp_callback_type_regex:
                if (regexec(callback->val.regex, path, callback->val.regex->re_nsub + 1, pmatch, 0) == 0) {
                    *start_offset = pmatch[callback->val.regex->re_nsub].rm_so;
                    *end_offset   = pmatch[callback->val.regex->re_nsub].rm_eo;

                    return callback;
                }

                break;
#endif
            case evhtp_callback_type_glob:
                if (_evhtp_glob_match(callback->val.glob, path) == 1) {
                    *start_offset = 0;
                    *end_offset   = (unsigned int)strlen(path);
                    return callback;
                }
            default:
                break;
        } /* switch */
    }

    return NULL;
}         /* _evhtp_callback_find */

/**
 * @brief Creates a new evhtp_request_t
 *
 * @param c
 *
 * @return evhtp_request_t structure on success, otherwise NULL
 */
static evhtp_request_t *
_evhtp_request_new(evhtp_connection_t * c) {
    evhtp_request_t * req;

    if (!(req = calloc(sizeof(evhtp_request_t), 1))) {
        return NULL;
    }

    req->conn        = c;
    req->htp         = c ? c->htp : NULL;
    req->status      = EVHTP_RES_OK;
    req->buffer_in   = evbuffer_new();
    req->buffer_out  = evbuffer_new();
    req->headers_in  = malloc(sizeof(evhtp_headers_t));
    req->headers_out = malloc(sizeof(evhtp_headers_t));

    TAILQ_INIT(req->headers_in);
    TAILQ_INIT(req->headers_out);

    return req;
}

/**
 * @brief frees all data in an evhtp_request_t along with calling finished hooks
 *
 * @param request the request structure
 */
static void
_evhtp_request_free(evhtp_request_t * request) {
    if (request == NULL) {
        return;
    }

    _evhtp_request_fini_hook(request);
    _evhtp_uri_free(request->uri);

    evhtp_headers_free(request->headers_in);
    evhtp_headers_free(request->headers_out);


    if (request->buffer_in) {
        evbuffer_free(request->buffer_in);
    }

    if (request->buffer_out) {
        evbuffer_free(request->buffer_out);
    }

    free(request->hooks);
    free(request);
}

/**
 * @brief create an overlay URI structure
 *
 * @return evhtp_uri_t
 */
static evhtp_uri_t *
_evhtp_uri_new(void) {
    evhtp_uri_t * uri;

    if (!(uri = calloc(sizeof(evhtp_uri_t), 1))) {
        return NULL;
    }

    return uri;
}

/**
 * @brief frees an overlay URI structure
 *
 * @param uri evhtp_uri_t
 */
static void
_evhtp_uri_free(evhtp_uri_t * uri) {
    if (uri == NULL) {
        return;
    }

    evhtp_query_free(uri->query);
    _evhtp_path_free(uri->path);

    free(uri->fragment);
    free(uri->query_raw);
    free(uri);
}

/**
 * @brief parses the path and file from an input buffer
 *
 * @details in order to properly create a structure that can match
 *          both a path and a file, this will parse a string into
 *          what it considers a path, and a file.
 *
 * @details if for example the input was "/a/b/c", the parser will
 *          consider "/a/b/" as the path, and "c" as the file.
 *
 * @param data raw input data (assumes a /path/[file] structure)
 * @param len length of the input data
 *
 * @return evhtp_request_t * on success, NULL on error.
 */
static evhtp_path_t *
_evhtp_path_new(const char * data, size_t len) {
    evhtp_path_t * req_path;
    const char   * data_end = (const char *)(data + len);
    char         * path     = NULL;
    char         * file     = NULL;

    if (!(req_path = calloc(sizeof(evhtp_path_t), 1))) {
        return NULL;
    }

    if (len == 0) {
        /*
         * odd situation here, no preceding "/", so just assume the path is "/"
         */
        path = strdup("/");
    } else if (*data != '/') {
        /* request like GET stupid HTTP/1.0, treat stupid as the file, and
         * assume the path is "/"
         */
        path = strdup("/");
        file = strndup(data, len);
    } else {
        if (data[len - 1] != '/') {
            /*
             * the last character in data is assumed to be a file, not the end of path
             * loop through the input data backwards until we find a "/"
             */
            size_t i;

            for (i = (len - 1); i != 0; i--) {
                if (data[i] == '/') {
                    /*
                     * we have found a "/" representing the start of the file,
                     * and the end of the path
                     */
                    size_t path_len;
                    size_t file_len;

                    path_len = (size_t)(&data[i] - data) + 1;
                    file_len = (size_t)(data_end - &data[i + 1]);

                    /* check for overflow */
                    if ((const char *)(data + path_len) > data_end) {
                        fprintf(stderr, "PATH Corrupted.. (path_len > len)\n");
                        free(req_path);
                        return NULL;
                    }

                    /* check for overflow */
                    if ((const char *)(&data[i + 1] + file_len) > data_end) {
                        fprintf(stderr, "FILE Corrupted.. (file_len > len)\n");
                        free(req_path);
                        return NULL;
                    }

                    path = strndup(data, path_len);
                    file = strndup(&data[i + 1], file_len);

                    break;
                }
            }

            if (i == 0 && data[i] == '/' && !file && !path) {
                /* drops here if the request is something like GET /foo */
                path = strdup("/");

                if (len > 1) {
                    file = strndup((const char *)(data + 1), len);
                }
            }
        } else {
            /* the last character is a "/", thus the request is just a path */
            path = strndup(data, len);
        }
    }

    if (len != 0) {
        req_path->full = strndup(data, len);
    }

    req_path->path = path;
    req_path->file = file;

    return req_path;
}     /* _evhtp_path_new */

static void
_evhtp_path_free(evhtp_path_t * path) {
    if (path == NULL) {
        return;
    }

    free(path->full);

    free(path->path);
    free(path->file);
    free(path->match_start);
    free(path->match_end);

    free(path);
}

static int
_evhtp_request_parser_start(htparser * p) {
    evhtp_connection_t * c = htparser_get_userdata(p);

    if (c->type == evhtp_type_client) {
        return 0;
    }

    if (c->paused == 1) {
        return -1;
    }

    if (c->request) {
        if (c->request->finished == 1) {
            _evhtp_request_free(c->request);
        } else {
            return -1;
        }
    }

    if (!(c->request = _evhtp_request_new(c))) {
        return -1;
    }

    return 0;
}

static int
_evhtp_request_parser_args(htparser * p, const char * data, size_t len) {
    evhtp_connection_t * c   = htparser_get_userdata(p);
    evhtp_uri_t        * uri = c->request->uri;

    if (c->type == evhtp_type_client) {
        /* as a client, technically we should never get here, but just in case
         * we return a 0 to the parser to continue.
         */
        return 0;
    }

    if (!(uri->query = evhtp_parse_query(data, len))) {
        c->request->status = EVHTP_RES_ERROR;
        return -1;
    }

    uri->query_raw = calloc(len + 1, 1);
    memcpy(uri->query_raw, data, len);

    return 0;
}

static int
_evhtp_request_parser_headers_start(htparser * p) {
    evhtp_connection_t * c = htparser_get_userdata(p);

    if ((c->request->status = _evhtp_headers_start_hook(c->request)) != EVHTP_RES_OK) {
        return -1;
    }

    return 0;
}

static int
_evhtp_request_parser_header_key(htparser * p, const char * data, size_t len) {
    evhtp_connection_t * c = htparser_get_userdata(p);
    char               * key_s;     /* = strndup(data, len); */
    evhtp_header_t     * hdr;

    key_s      = malloc(len + 1);
    key_s[len] = '\0';
    memcpy(key_s, data, len);

    if ((hdr = evhtp_header_key_add(c->request->headers_in, key_s, 0)) == NULL) {
        c->request->status = EVHTP_RES_FATAL;
        return -1;
    }

    hdr->k_heaped = 1;
    return 0;
}

static int
_evhtp_request_parser_header_val(htparser * p, const char * data, size_t len) {
    evhtp_connection_t * c = htparser_get_userdata(p);
    char               * val_s;
    evhtp_header_t     * header;

    val_s      = malloc(len + 1);
    val_s[len] = '\0';
    memcpy(val_s, data, len);

    if ((header = evhtp_header_val_add(c->request->headers_in, val_s, 0)) == NULL) {
        free(val_s);
        c->request->status = EVHTP_RES_FATAL;
        return -1;
    }

    header->v_heaped = 1;

    if ((c->request->status = _evhtp_header_hook(c->request, header)) != EVHTP_RES_OK) {
        return -1;
    }

    return 0;
}

static inline evhtp_t *
_evhtp_request_find_vhost(evhtp_t * evhtp, const char * name) {
    evhtp_t       * evhtp_vhost;
    evhtp_alias_t * evhtp_alias;

    TAILQ_FOREACH(evhtp_vhost, &evhtp->vhosts, next_vhost) {
        if (evhtp_vhost->server_name == NULL) {
            continue;
        }

        if (_evhtp_glob_match(evhtp_vhost->server_name, name) == 1) {
            return evhtp_vhost;
        }

        TAILQ_FOREACH(evhtp_alias, &evhtp_vhost->aliases, next) {
            if (evhtp_alias->alias == NULL) {
                continue;
            }

            if (_evhtp_glob_match(evhtp_alias->alias, name) == 1) {
                return evhtp_vhost;
            }
        }
    }

    return NULL;
}

static inline int
_evhtp_request_set_callbacks(evhtp_request_t * request) {
    evhtp_t            * evhtp;
    evhtp_connection_t * conn;
    evhtp_uri_t        * uri;
    evhtp_path_t       * path;
    evhtp_hooks_t      * hooks;
    evhtp_callback_t   * callback;
    evhtp_callback_cb    cb;
    void               * cbarg;

    if (request == NULL) {
        return -1;
    }

    if ((evhtp = request->htp) == NULL) {
        return -1;
    }

    if ((conn = request->conn) == NULL) {
        return -1;
    }

    if ((uri = request->uri) == NULL) {
        return -1;
    }

    if ((path = uri->path) == NULL) {
        return -1;
    }

    hooks    = NULL;
    callback = NULL;
    cb       = NULL;
    cbarg    = NULL;

    if ((callback = _evhtp_callback_find(evhtp->callbacks, path->full,
                                         &path->matched_soff, &path->matched_eoff))) {
        /* matched a callback using both path and file (/a/b/c/d) */
        cb    = callback->cb;
        cbarg = callback->cbarg;
        hooks = callback->hooks;
    } else if ((callback = _evhtp_callback_find(evhtp->callbacks, path->path,
                                                &path->matched_soff, &path->matched_eoff))) {
        /* matched a callback using *just* the path (/a/b/c/) */
        cb    = callback->cb;
        cbarg = callback->cbarg;
        hooks = callback->hooks;
    } else {
        /* no callbacks found for either case, use defaults */
        cb    = evhtp->defaults.cb;
        cbarg = evhtp->defaults.cbarg;

        path->matched_soff = 0;
        path->matched_eoff = (unsigned int)strlen(path->full);
    }

    if (path->match_start == NULL) {
        path->match_start = calloc(strlen(path->full) + 1, 1);
    }

    if (path->match_end == NULL) {
        path->match_end = calloc(strlen(path->full) + 1, 1);
    }

    if (path->matched_soff != UINT_MAX /*ONIG_REGION_NOTPOS*/) {
        if (path->matched_eoff - path->matched_soff) {
            memcpy(path->match_start, (void *)(path->full + path->matched_soff),
                   path->matched_eoff - path->matched_soff);
        } else {
            memcpy(path->match_start, (void *)(path->full + path->matched_soff),
                   strlen((const char *)(path->full + path->matched_soff)));
        }

        memcpy(path->match_end,
               (void *)(path->full + path->matched_eoff),
               strlen(path->full) - path->matched_eoff);
    }

    if (hooks != NULL) {
        if (request->hooks == NULL) {
            request->hooks = malloc(sizeof(evhtp_hooks_t));
        }

        memcpy(request->hooks, hooks, sizeof(evhtp_hooks_t));
    }

    request->cb    = cb;
    request->cbarg = cbarg;

    return 0;
} /* _evhtp_request_set_callbacks */

static int
_evhtp_request_parser_hostname(htparser * p, const char * data, size_t len) {
    evhtp_connection_t * c = htparser_get_userdata(p);
    evhtp_t            * evhtp;
    evhtp_t            * evhtp_vhost;

#ifndef EVHTP_DISABLE_SSL
    if (c->vhost_via_sni == 1 && c->ssl != NULL) {
        /* use the SNI set hostname instead of the header hostname */
        const char * host;

        host = SSL_get_servername(c->ssl, TLSEXT_NAMETYPE_host_name);

        if ((c->request->status = _evhtp_hostname_hook(c->request, host)) != EVHTP_RES_OK) {
            return -1;
        }

        return 0;
    }
#endif

    evhtp = c->htp;

    /* since this is called after _evhtp_request_parser_path(), which already
     * setup callbacks for the URI, we must now attempt to find callbacks which
     * are specific to this host.
     */
    _evhtp_lock(evhtp);
    {
        if ((evhtp_vhost = _evhtp_request_find_vhost(evhtp, data))) {
            _evhtp_lock(evhtp_vhost);
            {
                /* if we found a match for the host, we must set the htp
                 * variables for both the connection and the request.
                 */
                c->htp          = evhtp_vhost;
                c->request->htp = evhtp_vhost;

                _evhtp_request_set_callbacks(c->request);
            }
            _evhtp_unlock(evhtp_vhost);
        }
    }
    _evhtp_unlock(evhtp);

    if ((c->request->status = _evhtp_hostname_hook(c->request, data)) != EVHTP_RES_OK) {
        return -1;
    }

    return 0;
} /* _evhtp_request_parser_hostname */

static int
_evhtp_request_parser_path(htparser * p, const char * data, size_t len) {
    evhtp_connection_t * c = htparser_get_userdata(p);
    evhtp_uri_t        * uri;
    evhtp_path_t       * path;

    if (!(uri = _evhtp_uri_new())) {
        c->request->status = EVHTP_RES_FATAL;
        return -1;
    }

    if (!(path = _evhtp_path_new(data, len))) {
        _evhtp_uri_free(uri);
        c->request->status = EVHTP_RES_FATAL;
        return -1;
    }

    uri->path          = path;
    uri->scheme        = htparser_get_scheme(p);

    c->request->method = htparser_get_method(p);
    c->request->uri    = uri;

    _evhtp_lock(c->htp);
    {
        _evhtp_request_set_callbacks(c->request);
    }
    _evhtp_unlock(c->htp);

    if ((c->request->status = _evhtp_path_hook(c->request, path)) != EVHTP_RES_OK) {
        return -1;
    }

    return 0;
}     /* _evhtp_request_parser_path */

static int
_evhtp_request_parser_headers(htparser * p) {
    evhtp_connection_t * c = htparser_get_userdata(p);

    /* XXX proto should be set with htparsers on_hdrs_begin hook */
    c->request->keepalive = htparser_should_keep_alive(p);
    c->request->proto     = _evhtp_protocol(htparser_get_major(p), htparser_get_minor(p));
    c->request->status    = _evhtp_headers_hook(c->request, c->request->headers_in);

    if (c->request->status != EVHTP_RES_OK) {
        return -1;
    }

    if (c->type == evhtp_type_server && c->htp->disable_100_cont == 0) {
        /* only send a 100 continue response if it hasn't been disabled via
         * evhtp_disable_100_continue.
         */
        if (!evhtp_header_find(c->request->headers_in, "Expect")) {
            return 0;
        }

        evbuffer_add_printf(bufferevent_get_output(c->bev),
                            "HTTP/%d.%d 100 Continue\r\n\r\n",
                            htparser_get_major(p),
                            htparser_get_minor(p));
    }

    return 0;
}

static int
_evhtp_request_parser_body(htparser * p, const char * data, size_t len) {
    evhtp_connection_t * c   = htparser_get_userdata(p);
    evbuf_t            * buf;
    int                  res = 0;

    if (c->max_body_size > 0 && c->body_bytes_read + len >= c->max_body_size) {
        c->error           = 1;
        c->request->status = EVHTP_RES_DATA_TOO_LONG;

        return -1;
    }

    buf = evbuffer_new();
    evbuffer_add(buf, data, len);

    if ((c->request->status = _evhtp_body_hook(c->request, buf)) != EVHTP_RES_OK) {
        res = -1;
    }

    if (evbuffer_get_length(buf)) {
        evbuffer_add_buffer(c->request->buffer_in, buf);
    }

    evbuffer_free(buf);

    c->body_bytes_read += len;

    return res;
}

static int
_evhtp_request_parser_chunk_new(htparser * p) {
    evhtp_connection_t * c = htparser_get_userdata(p);

    if ((c->request->status = _evhtp_chunk_new_hook(c->request,
                                                    htparser_get_content_length(p))) != EVHTP_RES_OK) {
        return -1;
    }

    return 0;
}

static int
_evhtp_request_parser_chunk_fini(htparser * p) {
    evhtp_connection_t * c = htparser_get_userdata(p);

    if ((c->request->status = _evhtp_chunk_fini_hook(c->request)) != EVHTP_RES_OK) {
        return -1;
    }

    return 0;
}

static int
_evhtp_request_parser_chunks_fini(htparser * p) {
    evhtp_connection_t * c = htparser_get_userdata(p);

    if ((c->request->status = _evhtp_chunks_fini_hook(c->request)) != EVHTP_RES_OK) {
        return -1;
    }

    return 0;
}

/**
 * @brief determines if the request body contains the query arguments.
 *        if the query is NULL and the contenet length of the body has never
 *        been drained, and the content-type is x-www-form-urlencoded, the
 *        function returns 1
 *
 * @param req
 *
 * @return 1 if evhtp can use the body as the query arguments, 0 otherwise.
 */
static int
_evhtp_should_parse_query_body(evhtp_request_t * req) {
    const char * content_type;

    if (req == NULL) {
        return 0;
    }

    if (req->uri == NULL || req->uri->query != NULL) {
        return 0;
    }

    if (evhtp_request_content_len(req) == 0) {
        return 0;
    }

    if (evhtp_request_content_len(req) !=
        evbuffer_get_length(req->buffer_in)) {
        return 0;
    }

    content_type = evhtp_kv_find(req->headers_in, "content-type");

    if (content_type == NULL) {
        return 0;
    }

    if (strncasecmp(content_type, "application/x-www-form-urlencoded", 33)) {
        return 0;
    }

    return 1;
}

static int
_evhtp_request_parser_fini(htparser * p) {
    evhtp_connection_t * c = htparser_get_userdata(p);

    /* check to see if we should use the body of the request as the query
     * arguments.
     */
    if (_evhtp_should_parse_query_body(c->request) == 1) {
        const char  * body;
        size_t        body_len;
        evhtp_uri_t * uri;
        evbuf_t     * buf_in;

        uri            = c->request->uri;
        buf_in         = c->request->buffer_in;

        body_len       = evbuffer_get_length(buf_in);
        body           = (const char *)evbuffer_pullup(buf_in, body_len);

        uri->query_raw = calloc(body_len + 1, 1);
        memcpy(uri->query_raw, body, body_len);

        uri->query     = evhtp_parse_query(body, body_len);
    }


    /*
     * XXX c->request should never be NULL, but we have found some path of
     * execution where this actually happens. We will check for now, but the bug
     * path needs to be tracked down.
     *
     */
    if (c->request && c->request->cb) {
        (c->request->cb)(c->request, c->request->cbarg);
    }

    return 0;
}

static int
_evhtp_create_headers(evhtp_header_t * header, void * arg) {
    evbuf_t * buf = arg;

    evbuffer_add(buf, header->key, header->klen);
    evbuffer_add(buf, ": ", 2);
    evbuffer_add(buf, header->val, header->vlen);
    evbuffer_add(buf, "\r\n", 2);
    return 0;
}

static evbuf_t *
_evhtp_create_reply(evhtp_request_t * request, evhtp_res code) {
    evbuf_t    * buf          = evbuffer_new();
    const char * content_type = evhtp_header_find(request->headers_out, "Content-Type");
    char         res_buf[1024];
    int          sres;

    if (htparser_get_multipart(request->conn->parser) == 1) {
        goto check_proto;
    }

    if (evbuffer_get_length(request->buffer_out) && request->chunked == 0) {
        /* add extra headers (like content-length/type) if not already present */

        if (!evhtp_header_find(request->headers_out, "Content-Length")) {
            char lstr[128];
#ifndef WIN32
            sres = snprintf(lstr, sizeof(lstr), "%zu",
                            evbuffer_get_length(request->buffer_out));
#else
            sres = snprintf(lstr, sizeof(lstr), "%u",
                            evbuffer_get_length(request->buffer_out));
#endif

            if (sres >= sizeof(lstr) || sres < 0) {
                /* overflow condition, this should never happen, but if it does,
                 * well lets just shut the connection down */
                request->keepalive = 0;
                goto check_proto;
            }

            evhtp_headers_add_header(request->headers_out,
                                     evhtp_header_new("Content-Length", lstr, 0, 1));
        }

        if (!content_type) {
            evhtp_headers_add_header(request->headers_out,
                                     evhtp_header_new("Content-Type", "text/plain", 0, 0));
        }
    } else {
        if (!evhtp_header_find(request->headers_out, "Content-Length")) {
            const char * chunked = evhtp_header_find(request->headers_out,
                                                     "transfer-encoding");

            if (!chunked || !strstr(chunked, "chunked")) {
                evhtp_headers_add_header(request->headers_out,
                                         evhtp_header_new("Content-Length", "0", 0, 0));
            }
        }
    }

check_proto:
    /* add the proper keep-alive type headers based on http version */
    switch (request->proto) {
        case EVHTP_PROTO_11:
            if (request->keepalive == 0) {
                /* protocol is HTTP/1.1 but client wanted to close */
                evhtp_headers_add_header(request->headers_out,
                                         evhtp_header_new("Connection", "close", 0, 0));
            }
            break;
        case EVHTP_PROTO_10:
            if (request->keepalive == 1) {
                /* protocol is HTTP/1.0 and clients wants to keep established */
                evhtp_headers_add_header(request->headers_out,
                                         evhtp_header_new("Connection", "keep-alive", 0, 0));
            }
            break;
        default:
            /* this sometimes happens when a response is made but paused before
             * the method has been parsed */
            htparser_set_major(request->conn->parser, 1);
            htparser_set_minor(request->conn->parser, 0);
            break;
    } /* switch */


    /* attempt to add the status line into a temporary buffer and then use
     * evbuffer_add(). Using plain old snprintf() will be faster than
     * evbuffer_add_printf(). If the snprintf() fails, which it rarely should,
     * we fallback to using evbuffer_add_printf().
     */

    sres = snprintf(res_buf, sizeof(res_buf), "HTTP/%d.%d %d %s\r\n",
                    htparser_get_major(request->conn->parser),
                    htparser_get_minor(request->conn->parser),
                    code, status_code_to_str(code));

    if (sres >= sizeof(res_buf) || sres < 0) {
        /* failed to fit the whole thing in the res_buf, so just fallback to
         * using evbuffer_add_printf().
         */
        evbuffer_add_printf(buf, "HTTP/%d.%d %d %s\r\n",
                            htparser_get_major(request->conn->parser),
                            htparser_get_minor(request->conn->parser),
                            code, status_code_to_str(code));
    } else {
        /* copy the res_buf using evbuffer_add() instead of add_printf() */
        evbuffer_add(buf, res_buf, sres);
    }


    evhtp_headers_for_each(request->headers_out, _evhtp_create_headers, buf);
    evbuffer_add(buf, "\r\n", 2);

    if (evbuffer_get_length(request->buffer_out)) {
        evbuffer_add_buffer(buf, request->buffer_out);
    }

    return buf;
}     /* _evhtp_create_reply */

static void
_evhtp_connection_resumecb(int fd, short events, void * arg) {
    evhtp_connection_t * c = arg;

    c->paused = 0;

    bufferevent_enable(c->bev, EV_READ);

    if (c->request) {
        c->request->status = EVHTP_RES_OK;
    }

    if (c->free_connection == 1) {
        evhtp_connection_free(c);
        return;
    }

    _evhtp_connection_readcb(c->bev, c);
}

static void
_evhtp_connection_readcb(evbev_t * bev, void * arg) {
    evhtp_connection_t * c = arg;
    void               * buf;
    size_t               nread;
    size_t               avail;

    avail = evbuffer_get_length(bufferevent_get_input(bev));

    if (c->request) {
        c->request->status = EVHTP_RES_OK;
    }

    if (c->paused == 1) {
        return;
    }

    buf = evbuffer_pullup(bufferevent_get_input(bev), avail);

    bufferevent_disable(bev, EV_WRITE);
    {
        nread = htparser_run(c->parser, &request_psets, (const char *)buf, avail);
    }
    bufferevent_enable(bev, EV_WRITE);

    if (c->owner != 1) {
        /*
         * someone has taken the ownership of this connection, we still need to
         * drain the input buffer that had been read up to this point.
         */
        evbuffer_drain(bufferevent_get_input(bev), nread);
        evhtp_connection_free(c);
        return;
    }

    if (c->request) {
        switch (c->request->status) {
            case EVHTP_RES_DATA_TOO_LONG:
                if (c->request->hooks && c->request->hooks->on_error) {
                    (*c->request->hooks->on_error)(c->request, -1, c->request->hooks->on_error_arg);
                }
                evhtp_connection_free(c);
                return;
            default:
                break;
        }
    }

    evbuffer_drain(bufferevent_get_input(bev), nread);

    if (c->request && c->request->status == EVHTP_RES_PAUSE) {
        evhtp_request_pause(c->request);
    } else if (avail != nread) {
        evhtp_connection_free(c);
    }
} /* _evhtp_connection_readcb */

static void
_evhtp_connection_writecb(evbev_t * bev, void * arg) {
    evhtp_connection_t * c = arg;

    if (c->request == NULL) {
        return;
    }

    _evhtp_connection_write_hook(c);

    if (c->paused == 1) {
        return;
    }

    if (c->request->finished == 0 || evbuffer_get_length(bufferevent_get_output(bev))) {
        return;
    }

    /*
     * if there is a set maximum number of keepalive requests configured, check
     * to make sure we are not over it. If we have gone over the max we set the
     * keepalive bit to 0, thus closing the connection.
     */
    if (c->htp->max_keepalive_requests) {
        if (++c->num_requests >= c->htp->max_keepalive_requests) {
            c->request->keepalive = 0;
        }
    }

    if (c->request->keepalive) {
        _evhtp_request_free(c->request);

        c->request         = NULL;
        c->body_bytes_read = 0;

        if (c->htp->parent && c->vhost_via_sni == 0) {
            /* this request was servied by a virtual host evhtp_t structure
             * which was *NOT* found via SSL SNI lookup. In this case we want to
             * reset our connections evhtp_t structure back to the original so
             * that subsequent requests can have a different Host: header.
             */
            evhtp_t * orig_htp = c->htp->parent;

            c->htp = orig_htp;
        }

        htparser_init(c->parser, htp_type_request);


        htparser_set_userdata(c->parser, c);
        return;
    } else {
        evhtp_connection_free(c);
        return;
    }

    return;
} /* _evhtp_connection_writecb */

static void
_evhtp_connection_eventcb(evbev_t * bev, short events, void * arg) {
    evhtp_connection_t * c = arg;

    if ((events & BEV_EVENT_CONNECTED)) {
        if (c->type == evhtp_type_client) {
            bufferevent_setcb(bev,
                              _evhtp_connection_readcb,
                              _evhtp_connection_writecb,
                              _evhtp_connection_eventcb, c);
        }

        return;
    }

    if (c->ssl && !(events & BEV_EVENT_EOF)) {
        /* XXX need to do better error handling for SSL specific errors */
        c->error = 1;

        if (c->request) {
            c->request->error = 1;
        }
    }

    if (events == (BEV_EVENT_EOF | BEV_EVENT_READING)) {
        if (errno == EAGAIN) {
            /* libevent will sometimes recv again when it's not actually ready,
             * this results in a 0 return value, and errno will be set to EAGAIN
             * (try again). This does not mean there is a hard socket error, but
             * simply needs to be read again.
             *
             * but libevent will disable the read side of the bufferevent
             * anyway, so we must re-enable it.
             */
            bufferevent_enable(bev, EV_READ);
            errno = 0;
            return;
        }
    }

    c->error = 1;

    if (c->request && c->request->hooks && c->request->hooks->on_error) {
        (*c->request->hooks->on_error)(c->request, events,
                                       c->request->hooks->on_error_arg);
    }


    if (c->paused == 1) {
        c->free_connection = 1;
    } else {
        evhtp_connection_free((evhtp_connection_t *)arg);
    }
} /* _evhtp_connection_eventcb */

static int
_evhtp_run_pre_accept(evhtp_t * htp, evhtp_connection_t * conn) {
    void    * args;
    evhtp_res res;

    if (htp->defaults.pre_accept == NULL) {
        return 0;
    }

    args = htp->defaults.pre_accept_cbarg;
    res  = htp->defaults.pre_accept(conn, args);

    if (res != EVHTP_RES_OK) {
        return -1;
    }

    return 0;
}

static int
_evhtp_connection_accept(evbase_t * evbase, evhtp_connection_t * connection) {
    struct timeval * c_recv_timeo;
    struct timeval * c_send_timeo;

    if (_evhtp_run_pre_accept(connection->htp, connection) < 0) {
        evutil_closesocket(connection->sock);
        return -1;
    }

#ifndef EVHTP_DISABLE_SSL
    if (connection->htp->ssl_ctx != NULL) {
        connection->ssl = SSL_new(connection->htp->ssl_ctx);
        connection->bev = bufferevent_openssl_socket_new(evbase,
                                                         connection->sock,
                                                         connection->ssl,
                                                         BUFFEREVENT_SSL_ACCEPTING,
                                                         connection->htp->bev_flags);
        SSL_set_app_data(connection->ssl, connection);
        goto end;
    }
#endif

    connection->bev = bufferevent_socket_new(evbase,
                                             connection->sock,
                                             connection->htp->bev_flags);
#ifndef EVHTP_DISABLE_SSL
end:
#endif

    if (connection->recv_timeo.tv_sec || connection->recv_timeo.tv_usec) {
        c_recv_timeo = &connection->recv_timeo;
    } else if (connection->htp->recv_timeo.tv_sec ||
               connection->htp->recv_timeo.tv_usec) {
        c_recv_timeo = &connection->htp->recv_timeo;
    } else {
        c_recv_timeo = NULL;
    }

    if (connection->send_timeo.tv_sec || connection->send_timeo.tv_usec) {
        c_send_timeo = &connection->send_timeo;
    } else if (connection->htp->send_timeo.tv_sec ||
               connection->htp->send_timeo.tv_usec) {
        c_send_timeo = &connection->htp->send_timeo;
    } else {
        c_send_timeo = NULL;
    }

    evhtp_connection_set_timeouts(connection, c_recv_timeo, c_send_timeo);

    connection->resume_ev = event_new(evbase, -1, EV_READ | EV_PERSIST,
                                      _evhtp_connection_resumecb, connection);
    event_add(connection->resume_ev, NULL);

    bufferevent_enable(connection->bev, EV_READ);
    bufferevent_setcb(connection->bev,
                      _evhtp_connection_readcb,
                      _evhtp_connection_writecb,
                      _evhtp_connection_eventcb, connection);

    return 0;
}     /* _evhtp_connection_accept */

static void
_evhtp_default_request_cb(evhtp_request_t * request, void * arg) {
    evhtp_send_reply(request, EVHTP_RES_NOTFOUND);
}

static evhtp_connection_t *
_evhtp_connection_new(evhtp_t * htp, evutil_socket_t sock, evhtp_type type) {
    evhtp_connection_t * connection;
    htp_type             ptype;

    switch (type) {
        case evhtp_type_client:
            ptype = htp_type_response;
            break;
        case evhtp_type_server:
            ptype = htp_type_request;
            break;
        default:
            return NULL;
    }

    if (!(connection = calloc(sizeof(evhtp_connection_t), 1))) {
        return NULL;
    }

    connection->error  = 0;
    connection->owner  = 1;
    connection->paused = 0;
    connection->sock   = sock;
    connection->htp    = htp;
    connection->type   = type;
    connection->parser = htparser_new();

    htparser_init(connection->parser, ptype);
    htparser_set_userdata(connection->parser, connection);

    TAILQ_INIT(&connection->pending);

    return connection;
}

#ifdef LIBEVENT_HAS_SHUTDOWN
#ifndef EVHTP_DISABLE_SSL
static void
_evhtp_shutdown_eventcb(evbev_t * bev, short events, void * arg) {
}

#endif
#endif

static int
_evhtp_run_post_accept(evhtp_t * htp, evhtp_connection_t * connection) {
    void    * args;
    evhtp_res res;

    if (htp->defaults.post_accept == NULL) {
        return 0;
    }

    args = htp->defaults.post_accept_cbarg;
    res  = htp->defaults.post_accept(connection, args);

    if (res != EVHTP_RES_OK) {
        return -1;
    }

    return 0;
}

#ifndef EVHTP_DISABLE_EVTHR
static void
_evhtp_run_in_thread(evthr_t * thr, void * arg, void * shared) {
    evhtp_t            * htp        = shared;
    evhtp_connection_t * connection = arg;

    connection->evbase = evthr_get_base(thr);
    connection->thread = thr;

    evthr_inc_backlog(connection->thread);

    if (_evhtp_connection_accept(connection->evbase, connection) < 0) {
        evhtp_connection_free(connection);
        return;
    }

    if (_evhtp_run_post_accept(htp, connection) < 0) {
        evhtp_connection_free(connection);
        return;
    }
}

#endif

static void
_evhtp_accept_cb(evserv_t * serv, int fd, struct sockaddr * s, int sl, void * arg) {
    evhtp_t            * htp = arg;
    evhtp_connection_t * connection;

    if (!(connection = _evhtp_connection_new(htp, fd, evhtp_type_server))) {
        return;
    }

    connection->saddr = malloc(sl);
    memcpy(connection->saddr, s, sl);

#ifndef EVHTP_DISABLE_EVTHR
    if (htp->thr_pool != NULL) {
        if (evthr_pool_defer(htp->thr_pool, _evhtp_run_in_thread, connection) != EVTHR_RES_OK) {
            evutil_closesocket(connection->sock);
            evhtp_connection_free(connection);
            return;
        }
        return;
    }
#endif
    connection->evbase = htp->evbase;

    if (_evhtp_connection_accept(htp->evbase, connection) < 0) {
        evhtp_connection_free(connection);
        return;
    }

    if (_evhtp_run_post_accept(htp, connection) < 0) {
        evhtp_connection_free(connection);
        return;
    }
}

#ifndef EVHTP_DISABLE_SSL
#ifndef EVHTP_DISABLE_EVTHR
static unsigned long
_evhtp_ssl_get_thread_id(void) {
#ifndef WIN32
    return (unsigned long)pthread_self();
#else
    return (unsigned long)(pthread_self().p);
#endif
}

static void
_evhtp_ssl_thread_lock(int mode, int type, const char * file, int line) {
    if (type < ssl_num_locks) {
        if (mode & CRYPTO_LOCK) {
            pthread_mutex_lock(&(ssl_locks[type]));
        } else {
            pthread_mutex_unlock(&(ssl_locks[type]));
        }
    }
}

#endif
static void
_evhtp_ssl_delete_scache_ent(evhtp_ssl_ctx_t * ctx, evhtp_ssl_sess_t * sess) {
    evhtp_t         * htp;
    evhtp_ssl_cfg_t * cfg;
    unsigned char   * sid;
    unsigned int      slen;

    htp  = (evhtp_t *)SSL_CTX_get_app_data(ctx);
    cfg  = htp->ssl_cfg;

    sid  = sess->session_id;
    slen = sess->session_id_length;

    if (cfg->scache_del) {
        (cfg->scache_del)(htp, sid, slen);
    }
}

static int
_evhtp_ssl_add_scache_ent(evhtp_ssl_t * ssl, evhtp_ssl_sess_t * sess) {
    evhtp_connection_t * connection;
    evhtp_ssl_cfg_t    * cfg;
    unsigned char      * sid;
    int                  slen;

    connection = (evhtp_connection_t *)SSL_get_app_data(ssl);
    cfg        = connection->htp->ssl_cfg;

    sid        = sess->session_id;
    slen       = sess->session_id_length;

    SSL_set_timeout(sess, cfg->scache_timeout);

    if (cfg->scache_add) {
        return (cfg->scache_add)(connection, sid, slen, sess);
    }

    return 0;
}

static evhtp_ssl_sess_t *
_evhtp_ssl_get_scache_ent(evhtp_ssl_t * ssl, unsigned char * sid, int sid_len, int * copy) {
    evhtp_connection_t * connection;
    evhtp_ssl_cfg_t    * cfg;
    evhtp_ssl_sess_t   * sess;

    connection = (evhtp_connection_t * )SSL_get_app_data(ssl);
    cfg        = connection->htp->ssl_cfg;
    sess       = NULL;

    if (cfg->scache_get) {
        sess = (cfg->scache_get)(connection, sid, sid_len);
    }

    *copy = 0;

    return sess;
}

static int
_evhtp_ssl_servername(evhtp_ssl_t * ssl, int * unused, void * arg) {
    const char         * sname;
    evhtp_connection_t * connection;
    evhtp_t            * evhtp;
    evhtp_t            * evhtp_vhost;

    if (!(sname = SSL_get_servername(ssl, TLSEXT_NAMETYPE_host_name))) {
        return SSL_TLSEXT_ERR_NOACK;
    }

    if (!(connection = SSL_get_app_data(ssl))) {
        return SSL_TLSEXT_ERR_NOACK;
    }

    if (!(evhtp = connection->htp)) {
        return SSL_TLSEXT_ERR_NOACK;
    }

    if ((evhtp_vhost = _evhtp_request_find_vhost(evhtp, sname))) {
        connection->htp           = evhtp_vhost;
        connection->vhost_via_sni = 1;

        SSL_set_SSL_CTX(ssl, evhtp_vhost->ssl_ctx);
        SSL_set_options(ssl, SSL_CTX_get_options(ssl->ctx));

        if ((SSL_get_verify_mode(ssl) == SSL_VERIFY_NONE) ||
            (SSL_num_renegotiations(ssl) == 0)) {
            SSL_set_verify(ssl, SSL_CTX_get_verify_mode(ssl->ctx),
                           SSL_CTX_get_verify_callback(ssl->ctx));
        }

        return SSL_TLSEXT_ERR_OK;
    }

    return SSL_TLSEXT_ERR_NOACK;
} /* _evhtp_ssl_servername */

#endif

/*
 * PUBLIC FUNCTIONS
 */

htp_method
evhtp_request_get_method(evhtp_request_t * r) {
    return htparser_get_method(r->conn->parser);
}

/**
 * @brief pauses a connection (disables reading)
 *
 * @param c a evhtp_connection_t * structure
 */
void
evhtp_connection_pause(evhtp_connection_t * c) {
    if ((bufferevent_get_enabled(c->bev) & EV_READ)) {
        c->paused = 1;
        bufferevent_disable(c->bev, EV_READ);
    }
}

/**
 * @brief resumes a connection (enables reading) and activates resume event.
 *
 * @param c
 */
void
evhtp_connection_resume(evhtp_connection_t * c) {
    if (!(bufferevent_get_enabled(c->bev) & EV_READ)) {
        /* bufferevent_enable(c->bev, EV_READ); */
        c->paused = 0;
        event_active(c->resume_ev, EV_WRITE, 1);
    }
}

/**
 * @brief Wrapper around evhtp_connection_pause
 *
 * @see evhtp_connection_pause
 *
 * @param request
 */
void
evhtp_request_pause(evhtp_request_t * request) {
    request->status = EVHTP_RES_PAUSE;
    evhtp_connection_pause(request->conn);
}

/**
 * @brief Wrapper around evhtp_connection_resume
 *
 * @see evhtp_connection_resume
 *
 * @param request
 */
void
evhtp_request_resume(evhtp_request_t * request) {
    evhtp_connection_resume(request->conn);
}

evhtp_header_t *
evhtp_header_key_add(evhtp_headers_t * headers, const char * key, char kalloc) {
    evhtp_header_t * header;

    if (!(header = evhtp_header_new(key, NULL, kalloc, 0))) {
        return NULL;
    }

    evhtp_headers_add_header(headers, header);

    return header;
}

evhtp_header_t *
evhtp_header_val_add(evhtp_headers_t * headers, const char * val, char valloc) {
    evhtp_header_t * header;

    if (!headers || !val) {
        return NULL;
    }

    if (!(header = TAILQ_LAST(headers, evhtp_headers_s))) {
        return NULL;
    }

    if (header->val != NULL) {
        return NULL;
    }

    header->vlen = strlen(val);

    if (valloc == 1) {
        header->val = malloc(header->vlen + 1);
        header->val[header->vlen] = '\0';
        memcpy(header->val, val, header->vlen);
    } else {
        header->val = (char *)val;
    }

    header->v_heaped = valloc;

    return header;
}

evhtp_kvs_t *
evhtp_kvs_new(void) {
    evhtp_kvs_t * kvs = malloc(sizeof(evhtp_kvs_t));

    TAILQ_INIT(kvs);
    return kvs;
}

evhtp_kv_t *
evhtp_kv_new(const char * key, const char * val, char kalloc, char valloc) {
    evhtp_kv_t * kv;

    if (!(kv = malloc(sizeof(evhtp_kv_t)))) {
        return NULL;
    }

    kv->k_heaped = kalloc;
    kv->v_heaped = valloc;
    kv->klen     = 0;
    kv->vlen     = 0;
    kv->key      = NULL;
    kv->val      = NULL;

    if (key != NULL) {
        kv->klen = strlen(key);

        if (kalloc == 1) {
            char * s = malloc(kv->klen + 1);

            s[kv->klen] = '\0';
            memcpy(s, key, kv->klen);
            kv->key     = s;
        } else {
            kv->key = (char *)key;
        }
    }

    if (val != NULL) {
        kv->vlen = strlen(val);

        if (valloc == 1) {
            char * s = malloc(kv->vlen + 1);

            s[kv->vlen] = '\0';
            memcpy(s, val, kv->vlen);
            kv->val     = s;
        } else {
            kv->val = (char *)val;
        }
    }

    return kv;
}     /* evhtp_kv_new */

void
evhtp_kv_free(evhtp_kv_t * kv) {
    if (kv == NULL) {
        return;
    }

    if (kv->k_heaped) {
        free(kv->key);
    }

    if (kv->v_heaped) {
        free(kv->val);
    }

    free(kv);
}

void
evhtp_kv_rm_and_free(evhtp_kvs_t * kvs, evhtp_kv_t * kv) {
    if (kvs == NULL || kv == NULL) {
        return;
    }

    TAILQ_REMOVE(kvs, kv, next);

    evhtp_kv_free(kv);
}

void
evhtp_kvs_free(evhtp_kvs_t * kvs) {
    evhtp_kv_t * kv;
    evhtp_kv_t * save;

    if (kvs == NULL) {
        return;
    }

    for (kv = TAILQ_FIRST(kvs); kv != NULL; kv = save) {
        save = TAILQ_NEXT(kv, next);

        TAILQ_REMOVE(kvs, kv, next);

        evhtp_kv_free(kv);
    }

    free(kvs);
}

int
evhtp_kvs_for_each(evhtp_kvs_t * kvs, evhtp_kvs_iterator cb, void * arg) {
    evhtp_kv_t * kv;

    if (kvs == NULL || cb == NULL) {
        return -1;
    }

    TAILQ_FOREACH(kv, kvs, next) {
        int res;

        if ((res = cb(kv, arg))) {
            return res;
        }
    }

    return 0;
}

const char *
evhtp_kv_find(evhtp_kvs_t * kvs, const char * key) {
    evhtp_kv_t * kv;

    if (kvs == NULL || key == NULL) {
        return NULL;
    }

    TAILQ_FOREACH(kv, kvs, next) {
        if (strcasecmp(kv->key, key) == 0) {
            return kv->val;
        }
    }

    return NULL;
}

evhtp_kv_t *
evhtp_kvs_find_kv(evhtp_kvs_t * kvs, const char * key) {
    evhtp_kv_t * kv;

    if (kvs == NULL || key == NULL) {
        return NULL;
    }

    TAILQ_FOREACH(kv, kvs, next) {
        if (strcasecmp(kv->key, key) == 0) {
            return kv;
        }
    }

    return NULL;
}

void
evhtp_kvs_add_kv(evhtp_kvs_t * kvs, evhtp_kv_t * kv) {
    if (kvs == NULL || kv == NULL) {
        return;
    }

    TAILQ_INSERT_TAIL(kvs, kv, next);
}

void
evhtp_kvs_add_kvs(evhtp_kvs_t * dst, evhtp_kvs_t * src) {
    if (dst == NULL || src == NULL) {
        return;
    }

    evhtp_kv_t * kv;

    TAILQ_FOREACH(kv, src, next) {
        evhtp_kvs_add_kv(dst, evhtp_kv_new(kv->key, kv->val, kv->k_heaped, kv->v_heaped));
    }
}

typedef enum {
    s_query_start = 0,
    s_query_question_mark,
    s_query_separator,
    s_query_key,
    s_query_val,
    s_query_key_hex_1,
    s_query_key_hex_2,
    s_query_val_hex_1,
    s_query_val_hex_2,
    s_query_done
} query_parser_state;

static inline int
evhtp_is_hex_query_char(unsigned char ch) {
    switch (ch) {
        case 'a': case 'A':
        case 'b': case 'B':
        case 'c': case 'C':
        case 'd': case 'D':
        case 'e': case 'E':
        case 'f': case 'F':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return 1;
        default:
            return 0;
    } /* switch */
}

enum unscape_state {
    unscape_state_start = 0,
    unscape_state_hex1,
    unscape_state_hex2
};

int
evhtp_unescape_string(unsigned char ** out, unsigned char * str, size_t str_len) {
    unsigned char    * optr;
    unsigned char    * sptr;
    unsigned char      d;
    unsigned char      ch;
    unsigned char      c;
    size_t             i;
    enum unscape_state state;

    if (out == NULL || *out == NULL) {
        return -1;
    }

    state = unscape_state_start;
    optr  = *out;
    sptr  = str;
    d     = 0;

    for (i = 0; i < str_len; i++) {
        ch = *sptr++;

        switch (state) {
            case unscape_state_start:
                if (ch == '%') {
                    state = unscape_state_hex1;
                    break;
                }

                *optr++ = ch;

                break;
            case unscape_state_hex1:
                if (ch >= '0' && ch <= '9') {
                    d     = (unsigned char)(ch - '0');
                    state = unscape_state_hex2;
                    break;
                }

                c = (unsigned char)(ch | 0x20);

                if (c >= 'a' && c <= 'f') {
                    d     = (unsigned char)(c - 'a' + 10);
                    state = unscape_state_hex2;
                    break;
                }

                state   = unscape_state_start;
                *optr++ = ch;
                break;
            case unscape_state_hex2:
                state   = unscape_state_start;

                if (ch >= '0' && ch <= '9') {
                    ch      = (unsigned char)((d << 4) + ch - '0');

                    *optr++ = ch;
                    break;
                }

                c = (unsigned char)(ch | 0x20);

                if (c >= 'a' && c <= 'f') {
                    ch      = (unsigned char)((d << 4) + c - 'a' + 10);
                    *optr++ = ch;
                    break;
                }

                break;
        } /* switch */
    }

    return 0;
}         /* evhtp_unescape_string */

evhtp_query_t *
evhtp_parse_query(const char * query, size_t len) {
    evhtp_query_t    * query_args;
    query_parser_state state   = s_query_start;
    char             * key_buf = NULL;
    char             * val_buf = NULL;
    int                key_idx;
    int                val_idx;
    unsigned char      ch;
    size_t             i;

    query_args = evhtp_query_new();

    if (!(key_buf = malloc(len + 1))) {
        return NULL;
    }

    if (!(val_buf = malloc(len + 1))) {
        free(key_buf);
        return NULL;
    }

    key_idx = 0;
    val_idx = 0;

    for (i = 0; i < len; i++) {
        ch = query[i];

        if (key_idx >= len || val_idx >= len) {
            goto error;
        }

        switch (state) {
            case s_query_start:
                memset(key_buf, 0, len);
                memset(val_buf, 0, len);

                key_idx = 0;
                val_idx = 0;

                switch (ch) {
                    case '?':
                        state = s_query_key;
                        break;
                    case '/':
                        state = s_query_question_mark;
                        break;
                    default:
                        state = s_query_key;
                        goto query_key;
                }

                break;
            case s_query_question_mark:
                switch (ch) {
                    case '?':
                        state = s_query_key;
                        break;
                    case '/':
                        state = s_query_question_mark;
                        break;
                    default:
                        goto error;
                }
                break;
query_key:
            case s_query_key:
                switch (ch) {
                    case '=':
                        state = s_query_val;
                        break;
                    case '%':
                        key_buf[key_idx++] = ch;
                        key_buf[key_idx] = '\0';
                        state = s_query_key_hex_1;
                        break;
                    default:
                        key_buf[key_idx++] = ch;
                        key_buf[key_idx]   = '\0';
                        break;
                }
                break;
            case s_query_key_hex_1:
                if (!evhtp_is_hex_query_char(ch)) {
                    /* not hex, so we treat as a normal key */
                    if ((key_idx + 2) >= len) {
                        /* we need to insert \%<ch>, but not enough space */
                        goto error;
                    }

                    key_buf[key_idx - 1] = '%';
                    key_buf[key_idx++]   = ch;
                    key_buf[key_idx]     = '\0';
                    state = s_query_key;
                    break;
                }

                key_buf[key_idx++] = ch;
                key_buf[key_idx]   = '\0';

                state = s_query_key_hex_2;
                break;
            case s_query_key_hex_2:
                if (!evhtp_is_hex_query_char(ch)) {
                    goto error;
                }

                key_buf[key_idx++] = ch;
                key_buf[key_idx]   = '\0';

                state = s_query_key;
                break;
            case s_query_val:
                switch (ch) {
                    case ';':
                    case '&':
                        evhtp_kvs_add_kv(query_args, evhtp_kv_new(key_buf, val_buf, 1, 1));

                        memset(key_buf, 0, len);
                        memset(val_buf, 0, len);

                        key_idx            = 0;
                        val_idx            = 0;

                        state              = s_query_key;

                        break;
                    case '%':
                        val_buf[val_idx++] = ch;
                        val_buf[val_idx]   = '\0';

                        state              = s_query_val_hex_1;
                        break;
                    default:
                        val_buf[val_idx++] = ch;
                        val_buf[val_idx]   = '\0';

                        break;
                }     /* switch */
                break;
            case s_query_val_hex_1:
                if (!evhtp_is_hex_query_char(ch)) {
                    /* not really a hex val */
                    if ((val_idx + 2) >= len) {
                        /* we need to insert \%<ch>, but not enough space */
                        goto error;
                    }


                    val_buf[val_idx - 1] = '%';
                    val_buf[val_idx++]   = ch;
                    val_buf[val_idx]     = '\0';

                    state = s_query_val;
                    break;
                }

                val_buf[val_idx++] = ch;
                val_buf[val_idx]   = '\0';

                state = s_query_val_hex_2;
                break;
            case s_query_val_hex_2:
                if (!evhtp_is_hex_query_char(ch)) {
                    goto error;
                }

                val_buf[val_idx++] = ch;
                val_buf[val_idx]   = '\0';

                state = s_query_val;
                break;
            default:
                /* bad state */
                goto error;
        }       /* switch */
    }

    if (key_idx && val_idx) {
        evhtp_kvs_add_kv(query_args, evhtp_kv_new(key_buf, val_buf, 1, 1));
    }

    free(key_buf);
    free(val_buf);

    return query_args;
error:
    free(key_buf);
    free(val_buf);

    return NULL;
}     /* evhtp_parse_query */

void
evhtp_send_reply_start(evhtp_request_t * request, evhtp_res code) {
    evhtp_connection_t * c;
    evbuf_t            * reply_buf;

    c = evhtp_request_get_connection(request);

    if (!(reply_buf = _evhtp_create_reply(request, code))) {
        evhtp_connection_free(c);
        return;
    }

    bufferevent_write_buffer(c->bev, reply_buf);
    evbuffer_free(reply_buf);
}

void
evhtp_send_reply_body(evhtp_request_t * request, evbuf_t * buf) {
    evhtp_connection_t * c;

    c = request->conn;

    bufferevent_write_buffer(c->bev, buf);
}

void
evhtp_send_reply_end(evhtp_request_t * request) {
    request->finished = 1;

    _evhtp_connection_writecb(evhtp_request_get_bev(request),
                              evhtp_request_get_connection(request));
}

void
evhtp_send_reply(evhtp_request_t * request, evhtp_res code) {
    evhtp_connection_t * c;
    evbuf_t            * reply_buf;

    c = evhtp_request_get_connection(request);
    request->finished = 1;

    if (!(reply_buf = _evhtp_create_reply(request, code))) {
        evhtp_connection_free(request->conn);
        return;
    }

    bufferevent_write_buffer(evhtp_connection_get_bev(c), reply_buf);
    evbuffer_free(reply_buf);
}

int
evhtp_response_needs_body(const evhtp_res code, const htp_method method) {
    return code != EVHTP_RES_NOCONTENT &&
           code != EVHTP_RES_NOTMOD &&
           (code < 100 || code >= 200) &&
           method != htp_method_HEAD;
}

void
evhtp_send_reply_chunk_start(evhtp_request_t * request, evhtp_res code) {
    evhtp_header_t * content_len;

    if (evhtp_response_needs_body(code, request->method)) {
        content_len = evhtp_headers_find_header(request->headers_out, "Content-Length");

        switch (request->proto) {
            case EVHTP_PROTO_11:

                /*
                 * prefer HTTP/1.1 chunked encoding to closing the connection;
                 * note RFC 2616 section 4.4 forbids it with Content-Length:
                 * and it's not necessary then anyway.
                 */

                evhtp_kv_rm_and_free(request->headers_out, content_len);
                request->chunked = 1;
                break;
            case EVHTP_PROTO_10:
                /*
                 * HTTP/1.0 can be chunked as long as the Content-Length header
                 * is set to 0
                 */
                evhtp_kv_rm_and_free(request->headers_out, content_len);

                evhtp_headers_add_header(request->headers_out,
                                         evhtp_header_new("Content-Length", "0", 0, 0));

                request->chunked = 1;
                break;
            default:
                request->chunked = 0;
                break;
        } /* switch */
    } else {
        request->chunked = 0;
    }

    if (request->chunked == 1) {
        evhtp_headers_add_header(request->headers_out,
                                 evhtp_header_new("Transfer-Encoding", "chunked", 0, 0));

        /*
         * if data already exists on the output buffer, we automagically convert
         * it to the first chunk.
         */
        if (evbuffer_get_length(request->buffer_out) > 0) {
            char lstr[128];
            int  sres;

            sres = snprintf(lstr, sizeof(lstr), "%x\r\n",
                            (unsigned)evbuffer_get_length(request->buffer_out));

            if (sres >= sizeof(lstr) || sres < 0) {
                /* overflow condition, shouldn't ever get here, but lets
                 * terminate the connection asap */
                goto end;
            }

            evbuffer_prepend(request->buffer_out, lstr, strlen(lstr));
            evbuffer_add(request->buffer_out, "\r\n", 2);
        }
    }

end:
    evhtp_send_reply_start(request, code);
} /* evhtp_send_reply_chunk_start */

void
evhtp_send_reply_chunk(evhtp_request_t * request, evbuf_t * buf) {
    evbuf_t * output;

    output = bufferevent_get_output(request->conn->bev);

    if (evbuffer_get_length(buf) == 0) {
        return;
    }
    if (request->chunked) {
        evbuffer_add_printf(output, "%x\r\n",
                            (unsigned)evbuffer_get_length(buf));
    }
    evhtp_send_reply_body(request, buf);
    if (request->chunked) {
        evbuffer_add(output, "\r\n", 2);
    }
    bufferevent_flush(request->conn->bev, EV_WRITE, BEV_FLUSH);
}

void
evhtp_send_reply_chunk_end(evhtp_request_t * request) {
    if (request->chunked) {
        evbuffer_add(bufferevent_get_output(evhtp_request_get_bev(request)),
                     "0\r\n\r\n", 5);
    }

    evhtp_send_reply_end(request);
}

void
evhtp_unbind_socket(evhtp_t * htp) {
    evconnlistener_free(htp->server);
    htp->server = NULL;
}

int
evhtp_bind_sockaddr(evhtp_t * htp, struct sockaddr * sa, size_t sin_len, int backlog) {
#ifndef WIN32
    signal(SIGPIPE, SIG_IGN);
#endif

    htp->server = evconnlistener_new_bind(htp->evbase, _evhtp_accept_cb, (void *)htp,
                                          LEV_OPT_THREADSAFE | LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
                                          backlog, sa, sin_len);
    if (!htp->server) {
        return -1;
    }

#ifdef USE_DEFER_ACCEPT
    {
        evutil_socket_t sock;
        int             one = 1;

        sock = evconnlistener_get_fd(htp->server);

        setsockopt(sock, IPPROTO_TCP, TCP_DEFER_ACCEPT, &one, (ev_socklen_t)sizeof(one));
        setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &one, (ev_socklen_t)sizeof(one));
    }
#endif

#ifndef EVHTP_DISABLE_SSL
    if (htp->ssl_ctx != NULL) {
        /* if ssl is enabled and we have virtual hosts, set our servername
         * callback. We do this here because we want to make sure that this gets
         * set after all potential virtualhosts have been set, not just after
         * ssl_init.
         */
        if (TAILQ_FIRST(&htp->vhosts) != NULL) {
            SSL_CTX_set_tlsext_servername_callback(htp->ssl_ctx,
                                                   _evhtp_ssl_servername);
        }
    }
#endif

    return 0;
}

int
evhtp_bind_socket(evhtp_t * htp, const char * baddr, uint16_t port, int backlog) {
    struct sockaddr_in  sin;
    struct sockaddr_in6 sin6;

#ifndef NO_SYS_UN
    struct sockaddr_un sun;
#endif
    struct sockaddr  * sa;
    size_t             sin_len;

    memset(&sin, 0, sizeof(sin));

    if (!strncmp(baddr, "ipv6:", 5)) {
        memset(&sin6, 0, sizeof(sin6));

        baddr           += 5;
        sin_len          = sizeof(struct sockaddr_in6);
        sin6.sin6_port   = htons(port);
        sin6.sin6_family = AF_INET6;

        evutil_inet_pton(AF_INET6, baddr, &sin6.sin6_addr);
        sa = (struct sockaddr *)&sin6;
    } else if (!strncmp(baddr, "unix:", 5)) {
#ifndef NO_SYS_UN
        baddr += 5;

        if (strlen(baddr) >= sizeof(sun.sun_path)) {
            return -1;
        }

        memset(&sun, 0, sizeof(sun));

        sin_len        = sizeof(struct sockaddr_un);
        sun.sun_family = AF_UNIX;

        strncpy(sun.sun_path, baddr, strlen(baddr));

        sa = (struct sockaddr *)&sun;
#else
        fprintf(stderr, "System does not support AF_UNIX sockets\n");
        return -1;
#endif
    } else {
        if (!strncmp(baddr, "ipv4:", 5)) {
            baddr += 5;
        }

        sin_len             = sizeof(struct sockaddr_in);

        sin.sin_family      = AF_INET;
        sin.sin_port        = htons(port);
        sin.sin_addr.s_addr = inet_addr(baddr);

        sa = (struct sockaddr *)&sin;
    }

    return evhtp_bind_sockaddr(htp, sa, sin_len, backlog);
} /* evhtp_bind_socket */

void
evhtp_callbacks_free(evhtp_callbacks_t * callbacks) {
    evhtp_callback_t * callback;
    evhtp_callback_t * tmp;

    if (callbacks == NULL) {
        return;
    }

    TAILQ_FOREACH_SAFE(callback, callbacks, next, tmp) {
        TAILQ_REMOVE(callbacks, callback, next);

        evhtp_callback_free(callback);
    }

    free(callbacks);
}

evhtp_callback_t *
evhtp_callback_new(const char * path, evhtp_callback_type type, evhtp_callback_cb cb, void * arg) {
    evhtp_callback_t * hcb;

    if (!(hcb = calloc(sizeof(evhtp_callback_t), 1))) {
        return NULL;
    }

    hcb->type  = type;
    hcb->cb    = cb;
    hcb->cbarg = arg;

    switch (type) {
        case evhtp_callback_type_hash:
            hcb->hash      = _evhtp_quick_hash(path);
            hcb->val.path  = strdup(path);
            break;
#ifndef EVHTP_DISABLE_REGEX
        case evhtp_callback_type_regex:
            hcb->val.regex = malloc(sizeof(regex_t));

            if (regcomp(hcb->val.regex, (char *)path, REG_EXTENDED) != 0) {
                free(hcb->val.regex);
                free(hcb);
                return NULL;
            }
            break;
#endif
        case evhtp_callback_type_glob:
            hcb->val.glob = strdup(path);
            break;
        default:
            free(hcb);
            return NULL;
    } /* switch */

    return hcb;
}

void
evhtp_callback_free(evhtp_callback_t * callback) {
    if (callback == NULL) {
        return;
    }

    switch (callback->type) {
        case evhtp_callback_type_hash:
            free(callback->val.path);
            break;
        case evhtp_callback_type_glob:
            free(callback->val.glob);
            break;
#ifndef EVHTP_DISABLE_REGEX
        case evhtp_callback_type_regex:
            regfree(callback->val.regex);
            free(callback->val.regex);
            break;
#endif
    }

    if (callback->hooks) {
        free(callback->hooks);
    }

    free(callback);

    return;
}

int
evhtp_callbacks_add_callback(evhtp_callbacks_t * cbs, evhtp_callback_t * cb) {
    TAILQ_INSERT_TAIL(cbs, cb, next);

    return 0;
}

int
evhtp_set_hook(evhtp_hooks_t ** hooks, evhtp_hook_type type, evhtp_hook cb, void * arg) {
    if (*hooks == NULL) {
        if (!(*hooks = calloc(sizeof(evhtp_hooks_t), 1))) {
            return -1;
        }
    }

    switch (type) {
        case evhtp_hook_on_headers_start:
            (*hooks)->on_headers_start       = (evhtp_hook_headers_start_cb)cb;
            (*hooks)->on_headers_start_arg   = arg;
            break;
        case evhtp_hook_on_header:
            (*hooks)->on_header = (evhtp_hook_header_cb)cb;
            (*hooks)->on_header_arg          = arg;
            break;
        case evhtp_hook_on_headers:
            (*hooks)->on_headers             = (evhtp_hook_headers_cb)cb;
            (*hooks)->on_headers_arg         = arg;
            break;
        case evhtp_hook_on_path:
            (*hooks)->on_path = (evhtp_hook_path_cb)cb;
            (*hooks)->on_path_arg            = arg;
            break;
        case evhtp_hook_on_read:
            (*hooks)->on_read = (evhtp_hook_read_cb)cb;
            (*hooks)->on_read_arg            = arg;
            break;
        case evhtp_hook_on_request_fini:
            (*hooks)->on_request_fini        = (evhtp_hook_request_fini_cb)cb;
            (*hooks)->on_request_fini_arg    = arg;
            break;
        case evhtp_hook_on_connection_fini:
            (*hooks)->on_connection_fini     = (evhtp_hook_connection_fini_cb)cb;
            (*hooks)->on_connection_fini_arg = arg;
            break;
        case evhtp_hook_on_error:
            (*hooks)->on_error = (evhtp_hook_err_cb)cb;
            (*hooks)->on_error_arg           = arg;
            break;
        case evhtp_hook_on_new_chunk:
            (*hooks)->on_new_chunk           = (evhtp_hook_chunk_new_cb)cb;
            (*hooks)->on_new_chunk_arg       = arg;
            break;
        case evhtp_hook_on_chunk_complete:
            (*hooks)->on_chunk_fini          = (evhtp_hook_chunk_fini_cb)cb;
            (*hooks)->on_chunk_fini_arg      = arg;
            break;
        case evhtp_hook_on_chunks_complete:
            (*hooks)->on_chunks_fini         = (evhtp_hook_chunks_fini_cb)cb;
            (*hooks)->on_chunks_fini_arg     = arg;
            break;
        case evhtp_hook_on_hostname:
            (*hooks)->on_hostname            = (evhtp_hook_hostname_cb)cb;
            (*hooks)->on_hostname_arg        = arg;
            break;
        case evhtp_hook_on_write:
            (*hooks)->on_write = (evhtp_hook_write_cb)cb;
            (*hooks)->on_write_arg           = arg;
            break;
        default:
            return -1;
    }     /* switch */

    return 0;
}         /* evhtp_set_hook */

int
evhtp_unset_hook(evhtp_hooks_t ** hooks, evhtp_hook_type type) {
    return evhtp_set_hook(hooks, type, NULL, NULL);
}

int
evhtp_unset_all_hooks(evhtp_hooks_t ** hooks) {
    int res = 0;

    if (evhtp_unset_hook(hooks, evhtp_hook_on_headers_start)) {
        res -= 1;
    }

    if (evhtp_unset_hook(hooks, evhtp_hook_on_header)) {
        res -= 1;
    }

    if (evhtp_unset_hook(hooks, evhtp_hook_on_headers)) {
        res -= 1;
    }

    if (evhtp_unset_hook(hooks, evhtp_hook_on_path)) {
        res -= 1;
    }

    if (evhtp_unset_hook(hooks, evhtp_hook_on_read)) {
        res -= 1;
    }

    if (evhtp_unset_hook(hooks, evhtp_hook_on_request_fini)) {
        res -= 1;
    }

    if (evhtp_unset_hook(hooks, evhtp_hook_on_connection_fini)) {
        res -= 1;
    }

    if (evhtp_unset_hook(hooks, evhtp_hook_on_error)) {
        res -= 1;
    }

    if (evhtp_unset_hook(hooks, evhtp_hook_on_new_chunk)) {
        res -= 1;
    }

    if (evhtp_unset_hook(hooks, evhtp_hook_on_chunk_complete)) {
        res -= 1;
    }

    if (evhtp_unset_hook(hooks, evhtp_hook_on_chunks_complete)) {
        res -= 1;
    }

    if (evhtp_unset_hook(hooks, evhtp_hook_on_hostname)) {
        res -= 1;
    }

    if (evhtp_unset_hook(hooks, evhtp_hook_on_write)) {
        return -1;
    }

    return res;
} /* evhtp_unset_all_hooks */

evhtp_callback_t *
evhtp_set_cb(evhtp_t * htp, const char * path, evhtp_callback_cb cb, void * arg) {
    evhtp_callback_t * hcb;

    _evhtp_lock(htp);

    if (htp->callbacks == NULL) {
        if (!(htp->callbacks = calloc(sizeof(evhtp_callbacks_t), 1))) {
            _evhtp_unlock(htp);
            return NULL;
        }

        TAILQ_INIT(htp->callbacks);
    }

    if (!(hcb = evhtp_callback_new(path, evhtp_callback_type_hash, cb, arg))) {
        _evhtp_unlock(htp);
        return NULL;
    }

    if (evhtp_callbacks_add_callback(htp->callbacks, hcb)) {
        evhtp_callback_free(hcb);
        _evhtp_unlock(htp);
        return NULL;
    }

    _evhtp_unlock(htp);
    return hcb;
}

#ifndef EVHTP_DISABLE_EVTHR
static void
_evhtp_thread_init(evthr_t * thr, void * arg) {
    evhtp_t * htp = (evhtp_t *)arg;

    if (htp->thread_init_cb) {
        htp->thread_init_cb(htp, thr, htp->thread_init_cbarg);
    }
}

int
evhtp_use_threads(evhtp_t * htp, evhtp_thread_init_cb init_cb, int nthreads, void * arg) {
    htp->thread_init_cb    = init_cb;
    htp->thread_init_cbarg = arg;

#ifndef EVHTP_DISABLE_SSL
    evhtp_ssl_use_threads();
#endif

    if (!(htp->thr_pool = evthr_pool_new(nthreads, _evhtp_thread_init, htp))) {
        return -1;
    }

    evthr_pool_start(htp->thr_pool);
    return 0;
}

#endif

#ifndef EVHTP_DISABLE_EVTHR
int
evhtp_use_callback_locks(evhtp_t * htp) {
    if (htp == NULL) {
        return -1;
    }

    if (!(htp->lock = malloc(sizeof(pthread_mutex_t)))) {
        return -1;
    }

    return pthread_mutex_init(htp->lock, NULL);
}

#endif

#ifndef EVHTP_DISABLE_REGEX
evhtp_callback_t *
evhtp_set_regex_cb(evhtp_t * htp, const char * pattern, evhtp_callback_cb cb, void * arg) {
    evhtp_callback_t * hcb;

    _evhtp_lock(htp);

    if (htp->callbacks == NULL) {
        if (!(htp->callbacks = calloc(sizeof(evhtp_callbacks_t), 1))) {
            _evhtp_unlock(htp);
            return NULL;
        }

        TAILQ_INIT(htp->callbacks);
    }

    if (!(hcb = evhtp_callback_new(pattern, evhtp_callback_type_regex, cb, arg))) {
        _evhtp_unlock(htp);
        return NULL;
    }

    if (evhtp_callbacks_add_callback(htp->callbacks, hcb)) {
        evhtp_callback_free(hcb);
        _evhtp_unlock(htp);
        return NULL;
    }

    _evhtp_unlock(htp);
    return hcb;
}

#endif

evhtp_callback_t *
evhtp_set_glob_cb(evhtp_t * htp, const char * pattern, evhtp_callback_cb cb, void * arg) {
    evhtp_callback_t * hcb;

    _evhtp_lock(htp);

    if (htp->callbacks == NULL) {
        if (!(htp->callbacks = calloc(sizeof(evhtp_callbacks_t), 1))) {
            _evhtp_unlock(htp);
            return NULL;
        }

        TAILQ_INIT(htp->callbacks);
    }

    if (!(hcb = evhtp_callback_new(pattern, evhtp_callback_type_glob, cb, arg))) {
        _evhtp_unlock(htp);
        return NULL;
    }

    if (evhtp_callbacks_add_callback(htp->callbacks, hcb)) {
        evhtp_callback_free(hcb);
        _evhtp_unlock(htp);
        return NULL;
    }

    _evhtp_unlock(htp);
    return hcb;
}

void
evhtp_set_gencb(evhtp_t * htp, evhtp_callback_cb cb, void * arg) {
    htp->defaults.cb    = cb;
    htp->defaults.cbarg = arg;
}

void
evhtp_set_pre_accept_cb(evhtp_t * htp, evhtp_pre_accept_cb cb, void * arg) {
    htp->defaults.pre_accept       = cb;
    htp->defaults.pre_accept_cbarg = arg;
}

void
evhtp_set_post_accept_cb(evhtp_t * htp, evhtp_post_accept_cb cb, void * arg) {
    htp->defaults.post_accept       = cb;
    htp->defaults.post_accept_cbarg = arg;
}

#ifndef EVHTP_DISABLE_SSL
#ifndef EVHTP_DISABLE_EVTHR
int
evhtp_ssl_use_threads(void) {
    int i;

    if (ssl_locks_initialized == 1) {
        return 0;
    }

    ssl_locks_initialized = 1;

    ssl_num_locks         = CRYPTO_num_locks();
    ssl_locks = malloc(ssl_num_locks * sizeof(evhtp_mutex_t));

    for (i = 0; i < ssl_num_locks; i++) {
        pthread_mutex_init(&(ssl_locks[i]), NULL);
    }

    CRYPTO_set_id_callback(_evhtp_ssl_get_thread_id);
    CRYPTO_set_locking_callback(_evhtp_ssl_thread_lock);

    return 0;
}

#endif

int
evhtp_ssl_init(evhtp_t * htp, evhtp_ssl_cfg_t * cfg) {
#ifdef EVHTP_ENABLE_FUTURE_STUFF
    evhtp_ssl_scache_init init_cb = NULL;
    evhtp_ssl_scache_add  add_cb  = NULL;
    evhtp_ssl_scache_get  get_cb  = NULL;
    evhtp_ssl_scache_del  del_cb  = NULL;
#endif
    long                  cache_mode;

    if (cfg == NULL || htp == NULL || cfg->pemfile == NULL) {
        return -1;
    }

    SSL_library_init();
    SSL_load_error_strings();
    RAND_poll();

#if OPENSSL_VERSION_NUMBER < 0x10000000L
    STACK_OF(SSL_COMP) * comp_methods = SSL_COMP_get_compression_methods();
    sk_SSL_COMP_zero(comp_methods);
#endif

    htp->ssl_cfg = cfg;
    htp->ssl_ctx = SSL_CTX_new(SSLv23_server_method());

#if OPENSSL_VERSION_NUMBER >= 0x10000000L
    SSL_CTX_set_options(htp->ssl_ctx, SSL_MODE_RELEASE_BUFFERS | SSL_OP_NO_COMPRESSION);
    SSL_CTX_set_timeout(htp->ssl_ctx, cfg->ssl_ctx_timeout);
#endif

    SSL_CTX_set_options(htp->ssl_ctx, cfg->ssl_opts);

#ifndef OPENSSL_NO_ECDH
    if (cfg->named_curve != NULL) {
        EC_KEY * ecdh = NULL;
        int      nid  = 0;

        nid  = OBJ_sn2nid(cfg->named_curve);
        if (nid == 0) {
            fprintf(stderr, "ECDH initialization failed: unknown curve %s\n", cfg->named_curve);
        }
        ecdh = EC_KEY_new_by_curve_name(nid);
        if (ecdh == NULL) {
            fprintf(stderr, "ECDH initialization failed for curve %s\n", cfg->named_curve);
        }
        SSL_CTX_set_tmp_ecdh(htp->ssl_ctx, ecdh);
        EC_KEY_free(ecdh);
    }
#endif /* OPENSSL_NO_ECDH */
#ifndef OPENSSL_NO_DH
    if (cfg->dhparams != NULL) {
        FILE *fh;
        DH   *dh;

        fh = fopen(cfg->dhparams, "r");
        if (fh != NULL) {
            dh = PEM_read_DHparams(fh, NULL, NULL, NULL);
            if (dh != NULL) {
                SSL_CTX_set_tmp_dh(htp->ssl_ctx, dh);
                DH_free(dh);
            } else {
                fprintf(stderr, "DH initialization failed: unable to parse file %s\n", cfg->dhparams);
            }
            fclose(fh);
        } else {
            fprintf(stderr, "DH initialization failed: unable to open file %s\n", cfg->dhparams);
        }
    }
#endif /* OPENSSL_NO_DH */

    if (cfg->ciphers != NULL) {
        SSL_CTX_set_cipher_list(htp->ssl_ctx, cfg->ciphers);
    }

    SSL_CTX_load_verify_locations(htp->ssl_ctx, cfg->cafile, cfg->capath);
    X509_STORE_set_flags(SSL_CTX_get_cert_store(htp->ssl_ctx), cfg->store_flags);
    SSL_CTX_set_verify(htp->ssl_ctx, cfg->verify_peer, cfg->x509_verify_cb);

    if (cfg->x509_chk_issued_cb != NULL) {
        htp->ssl_ctx->cert_store->check_issued = cfg->x509_chk_issued_cb;
    }

    if (cfg->verify_depth) {
        SSL_CTX_set_verify_depth(htp->ssl_ctx, cfg->verify_depth);
    }

    switch (cfg->scache_type) {
        case evhtp_ssl_scache_type_disabled:
            cache_mode = SSL_SESS_CACHE_OFF;
            break;
#ifdef EVHTP_ENABLE_FUTURE_STUFF
        case evhtp_ssl_scache_type_user:
            cache_mode = SSL_SESS_CACHE_SERVER |
                         SSL_SESS_CACHE_NO_INTERNAL |
                         SSL_SESS_CACHE_NO_INTERNAL_LOOKUP;

            init_cb    = cfg->scache_init;
            add_cb     = cfg->scache_add;
            get_cb     = cfg->scache_get;
            del_cb     = cfg->scache_del;
            break;
        case evhtp_ssl_scache_type_builtin:
            cache_mode = SSL_SESS_CACHE_SERVER |
                         SSL_SESS_CACHE_NO_INTERNAL |
                         SSL_SESS_CACHE_NO_INTERNAL_LOOKUP;

            init_cb    = _evhtp_ssl_builtin_init;
            add_cb     = _evhtp_ssl_builtin_add;
            get_cb     = _evhtp_ssl_builtin_get;
            del_cb     = _evhtp_ssl_builtin_del;
            break;
#endif
        case evhtp_ssl_scache_type_internal:
        default:
            cache_mode = SSL_SESS_CACHE_SERVER;
            break;
    }     /* switch */

    SSL_CTX_use_certificate_file(htp->ssl_ctx, cfg->pemfile, SSL_FILETYPE_PEM);
    SSL_CTX_use_PrivateKey_file(htp->ssl_ctx,
                                cfg->privfile ? cfg->privfile : cfg->pemfile, SSL_FILETYPE_PEM);

    SSL_CTX_set_session_id_context(htp->ssl_ctx,
                                   (void *)&session_id_context,
                                   sizeof(session_id_context));

    SSL_CTX_set_app_data(htp->ssl_ctx, htp);
    SSL_CTX_set_session_cache_mode(htp->ssl_ctx, cache_mode);

    if (cache_mode != SSL_SESS_CACHE_OFF) {
        SSL_CTX_sess_set_cache_size(htp->ssl_ctx,
                                    cfg->scache_size ? cfg->scache_size : 1024);

        if (cfg->scache_type == evhtp_ssl_scache_type_builtin ||
            cfg->scache_type == evhtp_ssl_scache_type_user) {
            SSL_CTX_sess_set_new_cb(htp->ssl_ctx, _evhtp_ssl_add_scache_ent);
            SSL_CTX_sess_set_get_cb(htp->ssl_ctx, _evhtp_ssl_get_scache_ent);
            SSL_CTX_sess_set_remove_cb(htp->ssl_ctx, _evhtp_ssl_delete_scache_ent);

            if (cfg->scache_init) {
                cfg->args = (cfg->scache_init)(htp);
            }
        }
    }

    return 0;
}     /* evhtp_use_ssl */

#endif

evbev_t *
evhtp_connection_get_bev(evhtp_connection_t * connection) {
    return connection->bev;
}

evbev_t *
evhtp_connection_take_ownership(evhtp_connection_t * connection) {
    evbev_t * bev = evhtp_connection_get_bev(connection);

    if (connection->hooks) {
        evhtp_unset_all_hooks(&connection->hooks);
    }

    if (connection->request && connection->request->hooks) {
        evhtp_unset_all_hooks(&connection->request->hooks);
    }

    evhtp_connection_set_bev(connection, NULL);

    connection->owner = 0;

    bufferevent_disable(bev, EV_READ);
    bufferevent_setcb(bev, NULL, NULL, NULL, NULL);

    return bev;
}

evbev_t *
evhtp_request_get_bev(evhtp_request_t * request) {
    return evhtp_connection_get_bev(request->conn);
}

evbev_t *
evhtp_request_take_ownership(evhtp_request_t * request) {
    return evhtp_connection_take_ownership(evhtp_request_get_connection(request));
}

void
evhtp_connection_set_bev(evhtp_connection_t * conn, evbev_t * bev) {
    conn->bev = bev;
}

void
evhtp_request_set_bev(evhtp_request_t * request, evbev_t * bev) {
    evhtp_connection_set_bev(request->conn, bev);
}

evhtp_connection_t *
evhtp_request_get_connection(evhtp_request_t * request) {
    return request->conn;
}

inline void
evhtp_connection_set_timeouts(evhtp_connection_t   * c,
                              const struct timeval * rtimeo,
                              const struct timeval * wtimeo) {
    if (!c) {
        return;
    }

    if (rtimeo || wtimeo) {
        bufferevent_set_timeouts(c->bev, rtimeo, wtimeo);
    }
}

void
evhtp_connection_set_max_body_size(evhtp_connection_t * c, uint64_t len) {
    if (len == 0) {
        c->max_body_size = c->htp->max_body_size;
    } else {
        c->max_body_size = len;
    }
}

void
evhtp_request_set_max_body_size(evhtp_request_t * req, uint64_t len) {
    evhtp_connection_set_max_body_size(req->conn, len);
}

void
evhtp_connection_free(evhtp_connection_t * connection) {
    if (connection == NULL) {
        return;
    }

    _evhtp_request_free(connection->request);
    _evhtp_connection_fini_hook(connection);

    free(connection->parser);
    free(connection->hooks);
    free(connection->saddr);

    if (connection->resume_ev) {
        event_free(connection->resume_ev);
    }

    if (connection->bev) {
#ifdef LIBEVENT_HAS_SHUTDOWN
        bufferevent_shutdown(connection->bev, _evhtp_shutdown_eventcb);
#else
#ifndef EVHTP_DISABLE_SSL
        if (connection->ssl != NULL) {
            SSL_set_shutdown(connection->ssl, SSL_RECEIVED_SHUTDOWN);
            SSL_shutdown(connection->ssl);
        }
#endif
        bufferevent_free(connection->bev);
#endif
    }

#ifndef EVHTP_DISABLE_EVTHR
    if (connection->thread && connection->type == evhtp_type_server) {
        evthr_dec_backlog(connection->thread);
    }
#endif

    if (connection->ratelimit_cfg != NULL) {
        ev_token_bucket_cfg_free(connection->ratelimit_cfg);
    }

    free(connection);
}     /* evhtp_connection_free */

void
evhtp_request_free(evhtp_request_t * request) {
    _evhtp_request_free(request);
}

void
evhtp_set_timeouts(evhtp_t * htp, const struct timeval * r_timeo, const struct timeval * w_timeo) {
    if (r_timeo != NULL) {
        htp->recv_timeo = *r_timeo;
    }

    if (w_timeo != NULL) {
        htp->send_timeo = *w_timeo;
    }
}

void
evhtp_set_max_keepalive_requests(evhtp_t * htp, uint64_t num) {
    htp->max_keepalive_requests = num;
}

/**
 * @brief set bufferevent flags, defaults to BEV_OPT_CLOSE_ON_FREE
 *
 * @param htp
 * @param flags
 */
void
evhtp_set_bev_flags(evhtp_t * htp, int flags) {
    htp->bev_flags = flags;
}

void
evhtp_set_max_body_size(evhtp_t * htp, uint64_t len) {
    htp->max_body_size = len;
}

void
evhtp_disable_100_continue(evhtp_t * htp) {
    htp->disable_100_cont = 1;
}

int
evhtp_add_alias(evhtp_t * evhtp, const char * name) {
    evhtp_alias_t * alias;

    if (evhtp == NULL || name == NULL) {
        return -1;
    }

    if (!(alias = calloc(sizeof(evhtp_alias_t), 1))) {
        return -1;
    }

    alias->alias = strdup(name);

    TAILQ_INSERT_TAIL(&evhtp->aliases, alias, next);

    return 0;
}

/**
 * @brief add a virtual host.
 *
 * NOTE: If SSL is being used and the vhost was found via SNI, the Host: header
 *       will *NOT* be used to find a matching vhost.
 *
 *       Also, any hooks which are set prior to finding a vhost that are hooks
 *       which are after the host hook, they are overwritten by the callbacks
 *       and hooks set for the vhost specific evhtp_t structure.
 *
 * @param evhtp
 * @param name
 * @param vhost
 *
 * @return
 */
int
evhtp_add_vhost(evhtp_t * evhtp, const char * name, evhtp_t * vhost) {
    if (evhtp == NULL || name == NULL || vhost == NULL) {
        return -1;
    }

    if (TAILQ_FIRST(&vhost->vhosts) != NULL) {
        /* vhosts cannot have secondary vhosts defined */
        return -1;
    }

    if (!(vhost->server_name = strdup(name))) {
        return -1;
    }

    /* set the parent of this vhost so when the request has been completely
     * serviced, the vhost can be reset to the original evhtp structure.
     *
     * This allows for a keep-alive connection to make multiple requests with
     * different Host: values.
     */
    vhost->parent                 = evhtp;

    /* inherit various flags from the parent evhtp structure */
    vhost->bev_flags              = evhtp->bev_flags;
    vhost->max_body_size          = evhtp->max_body_size;
    vhost->max_keepalive_requests = evhtp->max_keepalive_requests;
    vhost->recv_timeo             = evhtp->recv_timeo;
    vhost->send_timeo             = evhtp->send_timeo;

    TAILQ_INSERT_TAIL(&evhtp->vhosts, vhost, next_vhost);

    return 0;
}

evhtp_t *
evhtp_new(evbase_t * evbase, void * arg) {
    evhtp_t * htp;

    if (evbase == NULL) {
        return NULL;
    }

    if (!(htp = calloc(sizeof(evhtp_t), 1))) {
        return NULL;
    }

    htp->arg       = arg;
    htp->evbase    = evbase;
    htp->bev_flags = BEV_OPT_CLOSE_ON_FREE;

    TAILQ_INIT(&htp->vhosts);
    TAILQ_INIT(&htp->aliases);

    evhtp_set_gencb(htp, _evhtp_default_request_cb, (void *)htp);

    return htp;
}

void
evhtp_free(evhtp_t * evhtp) {
    evhtp_alias_t * evhtp_alias, * tmp;

    if (evhtp == NULL) {
        return;
    }

#ifndef EVHTP_DISABLE_EVTHR
    if (evhtp->thr_pool) {
        evthr_pool_stop(evhtp->thr_pool);
        evthr_pool_free(evhtp->thr_pool);
    }
#endif

    if (evhtp->server_name) {
        free(evhtp->server_name);
    }

    if (evhtp->callbacks) {
        evhtp_callbacks_free(evhtp->callbacks);
    }

    TAILQ_FOREACH_SAFE(evhtp_alias, &evhtp->aliases, next, tmp) {
        if (evhtp_alias->alias != NULL) {
            free(evhtp_alias->alias);
        }
        TAILQ_REMOVE(&evhtp->aliases, evhtp_alias, next);
        free(evhtp_alias);
    }

#ifndef EVHTP_DISABLE_SSL
    if (evhtp->ssl_ctx) {
        SSL_CTX_free(evhtp->ssl_ctx);
    }
#endif

    free(evhtp);
}

int
evhtp_connection_set_rate_limit(evhtp_connection_t * conn,
                                size_t read_rate, size_t read_burst,
                                size_t write_rate, size_t write_burst,
                                const struct timeval * tick) {
    struct ev_token_bucket_cfg * tcfg;

    if (conn == NULL || conn->bev == NULL) {
        return -1;
    }

    tcfg = ev_token_bucket_cfg_new(read_rate, read_burst,
                                   write_rate, write_burst, tick);

    if (tcfg == NULL) {
        return -1;
    }

    conn->ratelimit_cfg = tcfg;

    return bufferevent_set_rate_limit(conn->bev, tcfg);
}

/*****************************************************************
* client request functions                                      *
*****************************************************************/

evhtp_connection_t *
evhtp_connection_new(evbase_t * evbase, const char * addr, uint16_t port) {
    evhtp_connection_t * conn;
    struct sockaddr_in   sin;

    if (evbase == NULL) {
        return NULL;
    }

    if (!(conn = _evhtp_connection_new(NULL, -1, evhtp_type_client))) {
        return NULL;
    }

    sin.sin_family      = AF_INET;
    sin.sin_addr.s_addr = inet_addr(addr);
    sin.sin_port        = htons(port);

    conn->evbase        = evbase;
    conn->bev           = bufferevent_socket_new(evbase, -1, BEV_OPT_CLOSE_ON_FREE);

    bufferevent_enable(conn->bev, EV_READ);

    bufferevent_setcb(conn->bev, NULL, NULL,
                      _evhtp_connection_eventcb, conn);

    bufferevent_socket_connect(conn->bev,
                               (struct sockaddr *)&sin, sizeof(sin));

    return conn;
}

evhtp_request_t *
evhtp_request_new(evhtp_callback_cb cb, void * arg) {
    evhtp_request_t * r;

    if (!(r = _evhtp_request_new(NULL))) {
        return NULL;
    }

    r->cb    = cb;
    r->cbarg = arg;
    r->proto = EVHTP_PROTO_11;

    return r;
}

int
evhtp_make_request(evhtp_connection_t * c, evhtp_request_t * r,
                   htp_method meth, const char * uri) {
    evbuf_t * obuf;
    char    * proto;

    obuf       = bufferevent_get_output(c->bev);
    r->conn    = c;
    c->request = r;

    switch (r->proto) {
        case EVHTP_PROTO_10:
            proto = "1.0";
            break;
        case EVHTP_PROTO_11:
        default:
            proto = "1.1";
            break;
    }

    evbuffer_add_printf(obuf, "%s %s HTTP/%s\r\n",
                        htparser_get_methodstr_m(meth), uri, proto);

    evhtp_headers_for_each(r->headers_out, _evhtp_create_headers, obuf);
    evbuffer_add_reference(obuf, "\r\n", 2, NULL, NULL);

    return 0;
}

unsigned int
evhtp_request_status(evhtp_request_t * r) {
    return htparser_get_status(r->conn->parser);
}

