#ifndef __EVHTP__H__
#define __EVHTP__H__

#include "evhtp-config.h"
#ifndef EVHTP_DISABLE_EVTHR
#include "evthr.h"
#endif

#include "htparse.h"

#ifndef EVHTP_DISABLE_REGEX
#include <onigposix.h>
#endif

#include <sys/queue.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

//#define EVHTP_DISABLE_SSL 1

#ifndef EVHTP_DISABLE_SSL
#include <event2/bufferevent_ssl.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef EVHTP_DISABLE_SSL
typedef SSL_SESSION               evhtp_ssl_sess_t;
typedef SSL                       evhtp_ssl_t;
typedef SSL_CTX                   evhtp_ssl_ctx_t;
typedef X509                      evhtp_x509_t;
typedef X509_STORE_CTX            evhtp_x509_store_ctx_t;
#else
typedef void                      evhtp_ssl_sess_t;
typedef void                      evhtp_ssl_t;
typedef void                      evhtp_ssl_ctx_t;
typedef void                      evhtp_x509_t;
typedef void                      evhtp_x509_store_ctx_t;
#endif

typedef struct evbuffer           evbuf_t;
typedef struct event              event_t;
typedef struct evconnlistener     evserv_t;
typedef struct bufferevent        evbev_t;

#ifdef EVHTP_DISABLE_EVTHR
typedef struct event_base         evbase_t;
typedef void                      evthr_t;
typedef void                      evthr_pool_t;
typedef void                      evhtp_mutex_t;
#else
typedef pthread_mutex_t           evhtp_mutex_t;
#endif

typedef struct evhtp_s            evhtp_t;
typedef struct evhtp_defaults_s   evhtp_defaults_t;
typedef struct evhtp_callbacks_s  evhtp_callbacks_t;
typedef struct evhtp_callback_s   evhtp_callback_t;
typedef struct evhtp_defaults_s   evhtp_defaults_5;
typedef struct evhtp_kv_s         evhtp_kv_t;
typedef struct evhtp_kvs_s        evhtp_kvs_t;
typedef struct evhtp_uri_s        evhtp_uri_t;
typedef struct evhtp_path_s       evhtp_path_t;
typedef struct evhtp_authority_s  evhtp_authority_t;
typedef struct evhtp_request_s    evhtp_request_t;
typedef struct evhtp_hooks_s      evhtp_hooks_t;
typedef struct evhtp_connection_s evhtp_connection_t;
typedef struct evhtp_ssl_cfg_s    evhtp_ssl_cfg_t;
typedef struct evhtp_alias_s      evhtp_alias_t;
typedef uint16_t                  evhtp_res;
typedef uint8_t                   evhtp_error_flags;


#define evhtp_header_s  evhtp_kv_s
#define evhtp_headers_s evhtp_kvs_s
#define evhtp_query_s   evhtp_kvs_s

#define evhtp_header_t  evhtp_kv_t
#define evhtp_headers_t evhtp_kvs_t
#define evhtp_query_t   evhtp_kvs_t

enum evhtp_ssl_scache_type {
    evhtp_ssl_scache_type_disabled = 0,
    evhtp_ssl_scache_type_internal,
    evhtp_ssl_scache_type_user,
    evhtp_ssl_scache_type_builtin
};

/**
 * @brief types associated with where a developer can hook into
 *        during the request processing cycle.
 */
enum evhtp_hook_type {
    evhtp_hook_on_header,       /**< type which defines to hook after one header has been parsed */
    evhtp_hook_on_headers,      /**< type which defines to hook after all headers have been parsed */
    evhtp_hook_on_path,         /**< type which defines to hook once a path has been parsed */
    evhtp_hook_on_read,         /**< type which defines to hook whenever the parser recieves data in a body */
    evhtp_hook_on_request_fini, /**< type which defines to hook before the request is free'd */
    evhtp_hook_on_connection_fini,
    evhtp_hook_on_new_chunk,
    evhtp_hook_on_chunk_complete,
    evhtp_hook_on_chunks_complete,
    evhtp_hook_on_headers_start,
    evhtp_hook_on_error,        /**< type which defines to hook whenever an error occurs */
    evhtp_hook_on_hostname,
    evhtp_hook_on_write
};

enum evhtp_callback_type {
    evhtp_callback_type_hash,
#ifndef EVHTP_DISABLE_REGEX
    evhtp_callback_type_regex,
#endif
    evhtp_callback_type_glob
};

enum evhtp_proto {
    EVHTP_PROTO_INVALID,
    EVHTP_PROTO_10,
    EVHTP_PROTO_11
};

enum evhtp_type {
    evhtp_type_client,
    evhtp_type_server
};

typedef enum evhtp_hook_type       evhtp_hook_type;
typedef enum evhtp_callback_type   evhtp_callback_type;
typedef enum evhtp_proto           evhtp_proto;
typedef enum evhtp_ssl_scache_type evhtp_ssl_scache_type;
typedef enum evhtp_type            evhtp_type;

typedef void (*evhtp_thread_init_cb)(evhtp_t * htp, evthr_t * thr, void * arg);
typedef void (*evhtp_callback_cb)(evhtp_request_t * req, void * arg);
typedef void (*evhtp_hook_err_cb)(evhtp_request_t * req, evhtp_error_flags errtype, void * arg);

/* Generic hook for passing ISO tests */
typedef evhtp_res (*evhtp_hook)();

typedef evhtp_res (*evhtp_pre_accept_cb)(evhtp_connection_t * conn, void * arg);
typedef evhtp_res (*evhtp_post_accept_cb)(evhtp_connection_t * conn, void * arg);
typedef evhtp_res (*evhtp_hook_header_cb)(evhtp_request_t * req, evhtp_header_t * hdr, void * arg);
typedef evhtp_res (*evhtp_hook_headers_cb)(evhtp_request_t * req, evhtp_headers_t * hdr, void * arg);
typedef evhtp_res (*evhtp_hook_path_cb)(evhtp_request_t * req, evhtp_path_t * path, void * arg);
typedef evhtp_res (*evhtp_hook_read_cb)(evhtp_request_t * req, evbuf_t * buf, void * arg);
typedef evhtp_res (*evhtp_hook_request_fini_cb)(evhtp_request_t * req, void * arg);
typedef evhtp_res (*evhtp_hook_connection_fini_cb)(evhtp_connection_t * connection, void * arg);
typedef evhtp_res (*evhtp_hook_chunk_new_cb)(evhtp_request_t * r, uint64_t len, void * arg);
typedef evhtp_res (*evhtp_hook_chunk_fini_cb)(evhtp_request_t * r, void * arg);
typedef evhtp_res (*evhtp_hook_chunks_fini_cb)(evhtp_request_t * r, void * arg);
typedef evhtp_res (*evhtp_hook_headers_start_cb)(evhtp_request_t * r, void * arg);
typedef evhtp_res (*evhtp_hook_hostname_cb)(evhtp_request_t * r, const char * hostname, void * arg);
typedef evhtp_res (*evhtp_hook_write_cb)(evhtp_connection_t * conn, void * arg);

typedef int (*evhtp_kvs_iterator)(evhtp_kv_t * kv, void * arg);
typedef int (*evhtp_headers_iterator)(evhtp_header_t * header, void * arg);

typedef int (*evhtp_ssl_verify_cb)(int pre_verify, evhtp_x509_store_ctx_t * ctx);
typedef int (*evhtp_ssl_chk_issued_cb)(evhtp_x509_store_ctx_t * ctx, evhtp_x509_t * x, evhtp_x509_t * issuer);

typedef int (*evhtp_ssl_scache_add)(evhtp_connection_t * connection, unsigned char * sid, int sid_len, evhtp_ssl_sess_t * sess);
typedef void (*evhtp_ssl_scache_del)(evhtp_t * htp, unsigned char * sid, int sid_len);
typedef evhtp_ssl_sess_t * (*evhtp_ssl_scache_get)(evhtp_connection_t * connection, unsigned char * sid, int sid_len);
typedef void * (*evhtp_ssl_scache_init)(evhtp_t *);

#define EVHTP_VERSION           "1.2.9"
#define EVHTP_VERSION_MAJOR     1
#define EVHTP_VERSION_MINOR     2
#define EVHTP_VERSION_PATCH     9

#define evhtp_headers_iterator  evhtp_kvs_iterator

#define EVHTP_RES_ERROR         0
#define EVHTP_RES_PAUSE         1
#define EVHTP_RES_FATAL         2
#define EVHTP_RES_USER          3
#define EVHTP_RES_DATA_TOO_LONG 4
#define EVHTP_RES_OK            200

#define EVHTP_RES_100           100
#define EVHTP_RES_CONTINUE      100
#define EVHTP_RES_SWITCH_PROTO  101
#define EVHTP_RES_PROCESSING    102
#define EVHTP_RES_URI_TOOLONG   122

#define EVHTP_RES_200           200
#define EVHTP_RES_CREATED       201
#define EVHTP_RES_ACCEPTED      202
#define EVHTP_RES_NAUTHINFO     203
#define EVHTP_RES_NOCONTENT     204
#define EVHTP_RES_RSTCONTENT    205
#define EVHTP_RES_PARTIAL       206
#define EVHTP_RES_MSTATUS       207
#define EVHTP_RES_IMUSED        226

#define EVHTP_RES_300           300
#define EVHTP_RES_MCHOICE       300
#define EVHTP_RES_MOVEDPERM     301
#define EVHTP_RES_FOUND         302
#define EVHTP_RES_SEEOTHER      303
#define EVHTP_RES_NOTMOD        304
#define EVHTP_RES_USEPROXY      305
#define EVHTP_RES_SWITCHPROXY   306
#define EVHTP_RES_TMPREDIR      307

#define EVHTP_RES_400           400
#define EVHTP_RES_BADREQ        400
#define EVHTP_RES_UNAUTH        401
#define EVHTP_RES_PAYREQ        402
#define EVHTP_RES_FORBIDDEN     403
#define EVHTP_RES_NOTFOUND      404
#define EVHTP_RES_METHNALLOWED  405
#define EVHTP_RES_NACCEPTABLE   406
#define EVHTP_RES_PROXYAUTHREQ  407
#define EVHTP_RES_TIMEOUT       408
#define EVHTP_RES_CONFLICT      409
#define EVHTP_RES_GONE          410
#define EVHTP_RES_LENREQ        411
#define EVHTP_RES_PRECONDFAIL   412
#define EVHTP_RES_ENTOOLARGE    413
#define EVHTP_RES_URITOOLARGE   414
#define EVHTP_RES_UNSUPPORTED   415
#define EVHTP_RES_RANGENOTSC    416
#define EVHTP_RES_EXPECTFAIL    417
#define EVHTP_RES_IAMATEAPOT    418

#define EVHTP_RES_500           500
#define EVHTP_RES_SERVERR       500
#define EVHTP_RES_NOTIMPL       501
#define EVHTP_RES_BADGATEWAY    502
#define EVHTP_RES_SERVUNAVAIL   503
#define EVHTP_RES_GWTIMEOUT     504
#define EVHTP_RES_VERNSUPPORT   505
#define EVHTP_RES_BWEXEED       509

struct evhtp_defaults_s {
    evhtp_callback_cb    cb;
    evhtp_pre_accept_cb  pre_accept;
    evhtp_post_accept_cb post_accept;
    void               * cbarg;
    void               * pre_accept_cbarg;
    void               * post_accept_cbarg;
};

struct evhtp_alias_s {
    char * alias;

    TAILQ_ENTRY(evhtp_alias_s) next;
};

/**
 * @brief main structure containing all configuration information
 */
struct evhtp_s {
    evhtp_t  * parent;           /**< only when this is a vhost */
    evbase_t * evbase;           /**< the initialized event_base */
    evserv_t * server;           /**< the libevent listener struct */
    char     * server_name;      /**< the name included in Host: responses */
    void     * arg;              /**< user-defined evhtp_t specific arguments */
    int        bev_flags;        /**< bufferevent flags to use on bufferevent_*_socket_new() */
    uint64_t   max_body_size;
    uint64_t   max_keepalive_requests;
    int        disable_100_cont; /**< if set, evhtp will not respond to Expect: 100-continue */

#ifndef EVHTP_DISABLE_SSL
    evhtp_ssl_ctx_t * ssl_ctx;   /**< if ssl enabled, this is the servers CTX */
    evhtp_ssl_cfg_t * ssl_cfg;
#endif

#ifndef EVHTP_DISABLE_EVTHR
    evthr_pool_t * thr_pool;     /**< connection threadpool */
#endif

#ifndef EVHTP_DISABLE_EVTHR
    pthread_mutex_t    * lock;   /**< parent lock for add/del cbs in threads */
    evhtp_thread_init_cb thread_init_cb;
    void               * thread_init_cbarg;
#endif
    evhtp_callbacks_t * callbacks;
    evhtp_defaults_t    defaults;

    struct timeval recv_timeo;
    struct timeval send_timeo;

    TAILQ_HEAD(, evhtp_alias_s) aliases;
    TAILQ_HEAD(, evhtp_s) vhosts;
    TAILQ_ENTRY(evhtp_s) next_vhost;
};

/**
 * @brief structure containing a single callback and configuration
 *
 * The definition structure which is used within the evhtp_callbacks_t
 * structure. This holds information about what should execute for either
 * a single or regex path.
 *
 * For example, if you registered a callback to be executed on a request
 * for "/herp/derp", your defined callback will be executed.
 *
 * Optionally you can set callback-specific hooks just like per-connection
 * hooks using the same rules.
 *
 */
struct evhtp_callback_s {
    evhtp_callback_type type;           /**< the type of callback (regex|path) */
    evhtp_callback_cb   cb;             /**< the actual callback function */
    unsigned int        hash;           /**< the full hash generated integer */
    void              * cbarg;          /**< user-defind arguments passed to the cb */
    evhtp_hooks_t     * hooks;          /**< per-callback hooks */

    union {
        char * path;
        char * glob;
#ifndef EVHTP_DISABLE_REGEX
        regex_t * regex;
#endif
    } val;

    TAILQ_ENTRY(evhtp_callback_s) next;
};

TAILQ_HEAD(evhtp_callbacks_s, evhtp_callback_s);

/**
 * @brief a generic key/value structure
 */
struct evhtp_kv_s {
    char * key;
    char * val;

    size_t klen;
    size_t vlen;

    char k_heaped; /**< set to 1 if the key can be free()'d */
    char v_heaped; /**< set to 1 if the val can be free()'d */

    TAILQ_ENTRY(evhtp_kv_s) next;
};

TAILQ_HEAD(evhtp_kvs_s, evhtp_kv_s);



/**
 * @brief a generic container representing an entire URI strucutre
 */
struct evhtp_uri_s {
    evhtp_authority_t * authority;
    evhtp_path_t      * path;
    unsigned char     * fragment;     /**< data after '#' in uri */
    unsigned char     * query_raw;    /**< the unparsed query arguments */
    evhtp_query_t     * query;        /**< list of k/v for query arguments */
    htp_scheme          scheme;       /**< set if a scheme is found */
};


/**
 * @brief structure which represents authority information in a URI
 */
struct evhtp_authority_s {
    char   * username;                /**< the username in URI (scheme://USER:.. */
    char   * password;                /**< the password in URI (scheme://...:PASS.. */
    char   * hostname;                /**< hostname if present in URI */
    uint16_t port;                    /**< port if present in URI */
};


/**
 * @brief structure which represents a URI path and or file
 */
struct evhtp_path_s {
    char       * full;                /**< the full path+file (/a/b/c.html) */
    char       * path;                /**< the path (/a/b/) */
    char       * file;                /**< the filename if present (c.html) */
    char       * match_start;
    char       * match_end;
    unsigned int matched_soff;        /**< offset of where the uri starts
                                       *   mainly used for regex matching
                                       */
    unsigned int matched_eoff;        /**< offset of where the uri ends
                                       *   mainly used for regex matching
                                       */
};


/**
 * @brief a structure containing all information for a http request.
 */
struct evhtp_request_s {
    evhtp_t            * htp;         /**< the parent evhtp_t structure */
    evhtp_connection_t * conn;        /**< the associated connection */
    evhtp_hooks_t      * hooks;       /**< request specific hooks */
    evhtp_uri_t        * uri;         /**< request URI information */
    evbuf_t            * buffer_in;   /**< buffer containing data from client */
    evbuf_t            * buffer_out;  /**< buffer containing data to client */
    evhtp_headers_t    * headers_in;  /**< headers from client */
    evhtp_headers_t    * headers_out; /**< headers to client */
    evhtp_proto          proto;       /**< HTTP protocol used */
    htp_method           method;      /**< HTTP method used */
    evhtp_res            status;      /**< The HTTP response code or other error conditions */
    int                  keepalive;   /**< set to 1 if the connection is keep-alive */
    int                  finished;    /**< set to 1 if the request is fully processed */
    int                  chunked;     /**< set to 1 if the request is chunked */

    evhtp_callback_cb cb;             /**< the function to call when fully processed */
    void            * cbarg;          /**< argument which is passed to the cb function */
    int               error;

    TAILQ_ENTRY(evhtp_request_s) next;
};

#define evhtp_request_content_len(r) htparser_get_content_length(r->conn->parser)

struct evhtp_connection_s {
    evhtp_t                    * htp;
    evbase_t                   * evbase;
    evbev_t                    * bev;
    evthr_t                    * thread;
    evhtp_ssl_t                * ssl;
    evhtp_hooks_t              * hooks;
    htparser                   * parser;
    event_t                    * resume_ev;
    struct sockaddr            * saddr;
    struct timeval               recv_timeo;    /**< conn read timeouts (overrides global) */
    struct timeval               send_timeo;    /**< conn write timeouts (overrides global) */
    evutil_socket_t              sock;
    uint8_t                      error;
    uint8_t                      owner;         /**< set to 1 if this structure owns the bufferevent */
    uint8_t                      vhost_via_sni; /**< set to 1 if the vhost was found via SSL SNI */
    evhtp_request_t            * request;       /**< the request currently being processed */
    uint64_t                     max_body_size;
    uint64_t                     body_bytes_read;
    uint64_t                     num_requests;
    evhtp_type                   type;          /**< server or client */
    char                         paused;
    char                         free_connection;
    struct ev_token_bucket_cfg * ratelimit_cfg; /**< connection-specific ratelimiting configuration. */

    TAILQ_HEAD(, evhtp_request_s) pending;      /**< client pending data */
};

struct evhtp_hooks_s {
    evhtp_hook_headers_start_cb   on_headers_start;
    evhtp_hook_header_cb          on_header;
    evhtp_hook_headers_cb         on_headers;
    evhtp_hook_path_cb            on_path;
    evhtp_hook_read_cb            on_read;
    evhtp_hook_request_fini_cb    on_request_fini;
    evhtp_hook_connection_fini_cb on_connection_fini;
    evhtp_hook_err_cb             on_error;
    evhtp_hook_chunk_new_cb       on_new_chunk;
    evhtp_hook_chunk_fini_cb      on_chunk_fini;
    evhtp_hook_chunks_fini_cb     on_chunks_fini;
    evhtp_hook_hostname_cb        on_hostname;
    evhtp_hook_write_cb           on_write;

    void * on_headers_start_arg;
    void * on_header_arg;
    void * on_headers_arg;
    void * on_path_arg;
    void * on_read_arg;
    void * on_request_fini_arg;
    void * on_connection_fini_arg;
    void * on_error_arg;
    void * on_new_chunk_arg;
    void * on_chunk_fini_arg;
    void * on_chunks_fini_arg;
    void * on_hostname_arg;
    void * on_write_arg;
};

struct evhtp_ssl_cfg_s {
    char                  * pemfile;
    char                  * privfile;
    char                  * cafile;
    char                  * capath;
    char                  * ciphers;
    char                  * named_curve;
    char                  * dhparams;
    long                    ssl_opts;
    long                    ssl_ctx_timeout;
    int                     verify_peer;
    int                     verify_depth;
    evhtp_ssl_verify_cb     x509_verify_cb;
    evhtp_ssl_chk_issued_cb x509_chk_issued_cb;
    long                    store_flags;
    evhtp_ssl_scache_type   scache_type;
    long                    scache_timeout;
    long                    scache_size;
    evhtp_ssl_scache_init   scache_init;
    evhtp_ssl_scache_add    scache_add;
    evhtp_ssl_scache_get    scache_get;
    evhtp_ssl_scache_del    scache_del;
    void                  * args;
};

/**
 * @brief creates a new evhtp_t instance
 *
 * @param evbase the initialized event base
 * @param arg user-defined argument which is evhtp_t specific
 *
 * @return a new evhtp_t structure or NULL on error
 */
evhtp_t * evhtp_new(evbase_t * evbase, void * arg);
void      evhtp_free(evhtp_t * evhtp);


/**
 * @brief set a read/write timeout on all things evhtp_t. When the timeout
 *        expires your error hook will be called with the libevent supplied event
 *        flags.
 *
 * @param htp the base evhtp_t struct
 * @param r read-timeout in timeval
 * @param w write-timeout in timeval.
 */
void evhtp_set_timeouts(evhtp_t * htp, const struct timeval * r, const struct timeval * w);
void evhtp_set_bev_flags(evhtp_t * htp, int flags);
int  evhtp_ssl_use_threads(void);
int  evhtp_ssl_init(evhtp_t * htp, evhtp_ssl_cfg_t * ssl_cfg);


/**
 * @brief when a client sends an Expect: 100-continue, if this is function is
 *        called, evhtp will not send a HTTP/x.x continue response.
 *
 * @param htp
 */
void evhtp_disable_100_continue(evhtp_t * htp);

/**
 * @brief creates a lock around callbacks and hooks, allowing for threaded
 * applications to add/remove/modify hooks & callbacks in a thread-safe manner.
 *
 * @param htp
 *
 * @return 0 on success, -1 on error
 */
int evhtp_use_callback_locks(evhtp_t * htp);

/**
 * @brief sets a callback which is called if no other callbacks are matched
 *
 * @param htp the initialized evhtp_t
 * @param cb  the function to be executed
 * @param arg user-defined argument passed to the callback
 */
void evhtp_set_gencb(evhtp_t * htp, evhtp_callback_cb cb, void * arg);
void evhtp_set_pre_accept_cb(evhtp_t * htp, evhtp_pre_accept_cb, void * arg);
void evhtp_set_post_accept_cb(evhtp_t * htp, evhtp_post_accept_cb, void * arg);


/**
 * @brief sets a callback to be executed on a specific path
 *
 * @param htp the initialized evhtp_t
 * @param path the path to match
 * @param cb the function to be executed
 * @param arg user-defined argument passed to the callback
 *
 * @return evhtp_callback_t * on success, NULL on error.
 */
evhtp_callback_t * evhtp_set_cb(evhtp_t * htp, const char * path, evhtp_callback_cb cb, void * arg);


/**
 * @brief sets a callback to be executed based on a regex pattern
 *
 * @param htp the initialized evhtp_t
 * @param pattern a POSIX compat regular expression
 * @param cb the function to be executed
 * @param arg user-defined argument passed to the callback
 *
 * @return evhtp_callback_t * on success, NULL on error
 */
#ifndef EVHTP_DISABLE_REGEX
evhtp_callback_t * evhtp_set_regex_cb(evhtp_t * htp, const char * pattern, evhtp_callback_cb cb, void * arg);
#endif



/**
 * @brief sets a callback to to be executed on simple glob/wildcard patterns
 *        this is useful if the app does not care about what was matched, but
 *        just that it matched. This is technically faster than regex.
 *
 * @param htp
 * @param pattern wildcard pattern, the '*' can be set at either or both the front or end.
 * @param cb
 * @param arg
 *
 * @return
 */
evhtp_callback_t * evhtp_set_glob_cb(evhtp_t * htp, const char * pattern, evhtp_callback_cb cb, void * arg);

/**
 * @brief sets a callback hook for either a connection or a path/regex .
 *
 * A user may set a variety of hooks either per-connection, or per-callback.
 * This allows the developer to hook into various parts of the request processing
 * cycle.
 *
 * a per-connection hook can be set at any time, but it is recommended to set these
 * during either a pre-accept phase, or post-accept phase. This allows a developer
 * to set hooks before any other hooks are called.
 *
 * a per-callback hook works differently. In this mode a developer can setup a set
 * of hooks prior to starting the event loop for specific callbacks. For example
 * if you wanted to hook something ONLY for a callback set by evhtp_set_cb or
 * evhtp_set_regex_cb this is the method of doing so.
 *
 * per-callback example:
 *
 * evhtp_callback_t * cb = evhtp_set_regex_cb(htp, "/anything/(.*)", default_cb, NULL);
 *
 * evhtp_set_hook(&cb->hooks, evhtp_hook_on_headers, anything_headers_cb, NULL);
 *
 * evhtp_set_hook(&cb->hooks, evhtp_hook_on_fini, anything_fini_cb, NULL);
 *
 * With the above example, once libevhtp has determined that it has a user-defined
 * callback for /anything/.*; anything_headers_cb will be executed after all headers
 * have been parsed, and anything_fini_cb will be executed before the request is
 * free()'d.
 *
 * The same logic applies to per-connection hooks, but it should be noted that if
 * a per-callback hook is set, the per-connection hook will be ignored.
 *
 * @param hooks double pointer to the evhtp_hooks_t structure
 * @param type the hook type
 * @param cb the callback to be executed.
 * @param arg optional argument which is passed when the callback is executed
 *
 * @return 0 on success, -1 on error (if hooks is NULL, it is allocated)
 */
int evhtp_set_hook(evhtp_hooks_t ** hooks, evhtp_hook_type type, evhtp_hook cb, void * arg);


/**
 * @brief remove a specific hook from being called.
 *
 * @param hooks
 * @param type
 *
 * @return
 */
int evhtp_unset_hook(evhtp_hooks_t ** hooks, evhtp_hook_type type);


/**
 * @brief removes all hooks.
 *
 * @param hooks
 *
 * @return
 */
int evhtp_unset_all_hooks(evhtp_hooks_t ** hooks);


/**
 * @brief bind to a socket, optionally with specific protocol support
 *        formatting. The addr can be defined as one of the following:
 *          ipv6:<ipv6addr> for binding to an IPv6 address.
 *          unix:<named pipe> for binding to a unix named socket
 *          ipv4:<ipv4addr> for binding to an ipv4 address
 *        Otherwise the addr is assumed to be ipv4.
 *
 * @param htp
 * @param addr
 * @param port
 * @param backlog
 *
 * @return
 */
int evhtp_bind_socket(evhtp_t * htp, const char * addr, uint16_t port, int backlog);


/**
 * @brief stops the listening socket.
 *
 * @param htp
 */
void evhtp_unbind_socket(evhtp_t * htp);

/**
 * @brief bind to an already allocated sockaddr.
 *
 * @param htp
 * @param
 * @param sin_len
 * @param backlog
 *
 * @return
 */
int  evhtp_bind_sockaddr(evhtp_t * htp, struct sockaddr *, size_t sin_len, int backlog);

int  evhtp_use_threads(evhtp_t * htp, evhtp_thread_init_cb init_cb, int nthreads, void * arg);
void evhtp_send_reply(evhtp_request_t * request, evhtp_res code);
void evhtp_send_reply_start(evhtp_request_t * request, evhtp_res code);
void evhtp_send_reply_body(evhtp_request_t * request, evbuf_t * buf);
void evhtp_send_reply_end(evhtp_request_t * request);

/**
 * @brief Determine if a response should have a body.
 * Follows the rules in RFC 2616 section 4.3.
 * @return 1 if the response MUST have a body; 0 if the response MUST NOT have
 *     a body.
 */
int evhtp_response_needs_body(const evhtp_res code, const htp_method method);


/**
 * @brief start a chunked response. If data already exists on the output buffer,
 *        this will be converted to the first chunk.
 *
 * @param request
 * @param code
 */
void evhtp_send_reply_chunk_start(evhtp_request_t * request, evhtp_res code);


/**
 * @brief send a chunk reply.
 *
 * @param request
 * @param buf
 */
void evhtp_send_reply_chunk(evhtp_request_t * request, evbuf_t * buf);


/**
 * @brief call when all chunks have been sent and you wish to send the last
 *        bits. This will add the last 0CRLFCRCL and call send_reply_end().
 *
 * @param request
 */
void evhtp_send_reply_chunk_end(evhtp_request_t * request);

/**
 * @brief creates a new evhtp_callback_t structure.
 *
 * All callbacks are stored in this structure
 * which define what the final function to be
 * called after all parsing is done. A callback
 * can be either a static string or a regular
 * expression.
 *
 * @param path can either be a static path (/path/to/resource/) or
 *        a POSIX compatible regular expression (^/resource/(.*))
 * @param type informs the function what type of of information is
 *        is contained within the path argument. This can either be
 *        callback_type_path, or callback_type_regex.
 * @param cb the callback function to be invoked
 * @param arg optional argument which is passed when the callback is executed.
 *
 * @return 0 on success, -1 on error.
 */
evhtp_callback_t * evhtp_callback_new(const char * path, evhtp_callback_type type, evhtp_callback_cb cb, void * arg);
void               evhtp_callback_free(evhtp_callback_t * callback);


/**
 * @brief Adds a evhtp_callback_t to the evhtp_callbacks_t list
 *
 * @param cbs an allocated evhtp_callbacks_t structure
 * @param cb  an initialized evhtp_callback_t structure
 *
 * @return 0 on success, -1 on error
 */
int evhtp_callbacks_add_callback(evhtp_callbacks_t * cbs, evhtp_callback_t * cb);


/**
 * @brief add an evhtp_t structure (with its own callbacks) to a base evhtp_t
 *        structure for virtual hosts. It should be noted that if you enable SSL
 *        on the base evhtp_t and your version of OpenSSL supports SNI, the SNI
 *        hostname will always take precedence over the Host header value.
 *
 * @param evhtp
 * @param name
 * @param vhost
 *
 * @return
 */
int evhtp_add_vhost(evhtp_t * evhtp, const char * name, evhtp_t * vhost);


/**
 * @brief Add an alias hostname for a virtual-host specific evhtp_t. This avoids
 *        having multiple evhtp_t virtual hosts with the same callback for the same
 *        vhost.
 *
 * @param evhtp
 * @param name
 *
 * @return
 */
int evhtp_add_alias(evhtp_t * evhtp, const char * name);

/**
 * @brief Allocates a new key/value structure.
 *
 * @param key null terminated string
 * @param val null terminated string
 * @param kalloc if set to 1, the key will be copied, if 0 no copy is done.
 * @param valloc if set to 1, the val will be copied, if 0 no copy is done.
 *
 * @return evhtp_kv_t * on success, NULL on error.
 */
evhtp_kv_t  * evhtp_kv_new(const char * key, const char * val, char kalloc, char valloc);
evhtp_kvs_t * evhtp_kvs_new(void);

void          evhtp_kv_free(evhtp_kv_t * kv);
void          evhtp_kvs_free(evhtp_kvs_t * kvs);
void          evhtp_kv_rm_and_free(evhtp_kvs_t * kvs, evhtp_kv_t * kv);

const char  * evhtp_kv_find(evhtp_kvs_t * kvs, const char * key);
evhtp_kv_t  * evhtp_kvs_find_kv(evhtp_kvs_t * kvs, const char * key);


/**
 * @brief appends a key/val structure to a evhtp_kvs_t tailq
 *
 * @param kvs an evhtp_kvs_t structure
 * @param kv  an evhtp_kv_t structure
 */
void evhtp_kvs_add_kv(evhtp_kvs_t * kvs, evhtp_kv_t * kv);

/**
 * @brief appends all key/val structures from src tailq onto dst tailq
 *
 * @param dst an evhtp_kvs_t structure
 * @param src an evhtp_kvs_t structure
 */
void evhtp_kvs_add_kvs(evhtp_kvs_t * dst, evhtp_kvs_t * src);

int  evhtp_kvs_for_each(evhtp_kvs_t * kvs, evhtp_kvs_iterator cb, void * arg);

/**
 * @brief Parses the query portion of the uri into a set of key/values
 *
 * Parses query arguments like "?herp=derp&foo=bar;blah=baz"
 *
 * @param query data containing the uri query arguments
 * @param len size of the data
 *
 * @return evhtp_query_t * on success, NULL on error
 */
evhtp_query_t * evhtp_parse_query(const char * query, size_t len);


/**
 * @brief Unescapes strings like '%7B1,%202,%203%7D' would become '{1, 2, 3}'
 *
 * @param out double pointer where output is stored. This is allocated by the user.
 * @param str the string to unescape
 * @param str_len the length of the string to unescape
 *
 * @return 0 on success, -1 on error
 */
int evhtp_unescape_string(unsigned char ** out, unsigned char * str, size_t str_len);

/**
 * @brief creates a new evhtp_header_t key/val structure
 *
 * @param key a null terminated string
 * @param val a null terminated string
 * @param kalloc if 1, key will be copied, otherwise no copy performed
 * @param valloc if 1, val will be copied, otehrwise no copy performed
 *
 * @return evhtp_header_t * or NULL on error
 */
evhtp_header_t * evhtp_header_new(const char * key, const char * val, char kalloc, char valloc);

/**
 * @brief creates a new evhtp_header_t, sets only the key, and adds to the
 *        evhtp_headers TAILQ
 *
 * @param headers the evhtp_headers_t TAILQ (evhtp_kv_t)
 * @param key a null terminated string
 * @param kalloc if 1 the string will be copied, otherwise assigned
 *
 * @return an evhtp_header_t pointer or NULL on error
 */
evhtp_header_t * evhtp_header_key_add(evhtp_headers_t * headers, const char * key, char kalloc);


/**
 * @brief finds the last header in the headers tailq and adds the value
 *
 * @param headers the evhtp_headers_t TAILQ (evhtp_kv_t)
 * @param val a null terminated string
 * @param valloc if 1 the string will be copied, otherwise assigned
 *
 * @return an evhtp_header_t pointer or NULL on error
 */
evhtp_header_t * evhtp_header_val_add(evhtp_headers_t * headers, const char * val, char valloc);


/**
 * @brief adds an evhtp_header_t to the end of the evhtp_headers_t tailq
 *
 * @param headers
 * @param header
 */
void evhtp_headers_add_header(evhtp_headers_t * headers, evhtp_header_t * header);

/**
 * @brief finds the value of a key in a evhtp_headers_t structure
 *
 * @param headers the evhtp_headers_t tailq
 * @param key the key to find
 *
 * @return the value of the header key if found, NULL if not found.
 */
const char * evhtp_header_find(evhtp_headers_t * headers, const char * key);

#define evhtp_header_find         evhtp_kv_find
#define evhtp_headers_find_header evhtp_kvs_find_kv
#define evhtp_headers_for_each    evhtp_kvs_for_each
#define evhtp_header_new          evhtp_kv_new
#define evhtp_header_free         evhtp_kv_free
#define evhtp_headers_new         evhtp_kvs_new
#define evhtp_headers_free        evhtp_kvs_free
#define evhtp_header_rm_and_free  evhtp_kv_rm_and_free
#define evhtp_headers_add_header  evhtp_kvs_add_kv
#define evhtp_headers_add_headers evhtp_kvs_add_kvs
#define evhtp_query_new           evhtp_kvs_new
#define evhtp_query_free          evhtp_kvs_free


/**
 * @brief returns the htp_method enum version of the request method.
 *
 * @param r
 *
 * @return htp_method enum
 */
htp_method evhtp_request_get_method(evhtp_request_t * r);

void       evhtp_connection_pause(evhtp_connection_t * connection);
void       evhtp_connection_resume(evhtp_connection_t * connection);
void       evhtp_request_pause(evhtp_request_t * request);
void       evhtp_request_resume(evhtp_request_t * request);


/**
 * @brief returns the underlying evhtp_connection_t structure from a request
 *
 * @param request
 *
 * @return evhtp_connection_t on success, otherwise NULL
 */
evhtp_connection_t * evhtp_request_get_connection(evhtp_request_t * request);

/**
 * @brief Sets the connections underlying bufferevent
 *
 * @param conn
 * @param bev
 */
void evhtp_connection_set_bev(evhtp_connection_t * conn, evbev_t * bev);

/**
 * @brief sets the underlying bufferevent for a evhtp_request
 *
 * @param request
 * @param bev
 */
void evhtp_request_set_bev(evhtp_request_t * request, evbev_t * bev);


/**
 * @brief returns the underlying connections bufferevent
 *
 * @param conn
 *
 * @return bufferevent on success, otherwise NULL
 */
evbev_t * evhtp_connection_get_bev(evhtp_connection_t * conn);


/**
 * @brief sets a connection-specific read/write timeout which overrides the
 *        global read/write settings.
 *
 * @param conn
 * @param r timeval for read
 * @param w timeval for write
 */
void evhtp_connection_set_timeouts(evhtp_connection_t * conn, const struct timeval * r, const struct timeval * w);

/**
 * @brief returns the underlying requests bufferevent
 *
 * @param request
 *
 * @return bufferevent on success, otherwise NULL
 */
evbev_t * evhtp_request_get_bev(evhtp_request_t * request);


/**
 * @brief let a user take ownership of the underlying bufferevent and free
 *        all other underlying resources.
 *
 * Warning: this will free all evhtp_connection/request structures, remove all
 * associated hooks and reset the bufferevent to defaults, i.e., disable
 * EV_READ, and set all callbacks to NULL.
 *
 * @param connection
 *
 * @return underlying connections bufferevent.
 */
evbev_t * evhtp_connection_take_ownership(evhtp_connection_t * connection);


/**
 * @brief free's all connection related resources, this will also call your
 *        request fini hook and request fini hook.
 *
 * @param connection
 */
void evhtp_connection_free(evhtp_connection_t * connection);

void evhtp_request_free(evhtp_request_t * request);

/**
 * @brief set a max body size to accept for an incoming request, this will
 *        default to unlimited.
 *
 * @param htp
 * @param len
 */
void evhtp_set_max_body_size(evhtp_t * htp, uint64_t len);


/**
 * @brief set a max body size for a specific connection, this will default to
 *        the size set by evhtp_set_max_body_size
 *
 * @param conn
 * @param len
 */
void evhtp_connection_set_max_body_size(evhtp_connection_t * conn, uint64_t len);

/**
 * @brief just calls evhtp_connection_set_max_body_size for the request.
 *
 * @param request
 * @param len
 */
void evhtp_request_set_max_body_size(evhtp_request_t * request, uint64_t len);

/**
 * @brief sets a maximum number of requests that a single connection can make.
 *
 * @param htp
 * @param num
 */
void evhtp_set_max_keepalive_requests(evhtp_t * htp, uint64_t num);


/**
 * @brief set a bufferevent ratelimit on a evhtp_connection_t structure. The
 *        logic is the same as libevent's rate-limiting code.
 *
 * @param c
 * @param read_rate
 * @param read_burst
 * @param write_rate
 * @param write_burst
 * @param tick
 *
 * @return
 */
int evhtp_connection_set_ratelimit(evhtp_connection_t * c, size_t read_rate,
    size_t read_burst, size_t write_rate, size_t write_burst, const struct timeval * tick);

/*****************************************************************
* client request functions                                      *
*****************************************************************/

/**
 * @brief allocate a new connection
 */
evhtp_connection_t * evhtp_connection_new(evbase_t * evbase, const char * addr, uint16_t port);

/**
 * @brief allocate a new request
 */
evhtp_request_t * evhtp_request_new(evhtp_callback_cb cb, void * arg);

/**
 * @brief make a client request
 */
int          evhtp_make_request(evhtp_connection_t * c, evhtp_request_t * r, htp_method meth, const char * uri);

unsigned int evhtp_request_status(evhtp_request_t *);

#ifdef __cplusplus
}
#endif

#endif /* __EVHTP__H__ */
