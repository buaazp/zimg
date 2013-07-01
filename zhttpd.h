#include "zcommon.h"

static const char * guess_content_type(const char *path);
static void dump_request_cb(struct evhttp_request *req, void *arg);
static void post_request_cb(struct evhttp_request *req, void *arg);
static void zimg_cb(struct evhttp_request *req, void *arg);
static void send_document_cb(struct evhttp_request *req, void *arg);
static int displayAddress(struct evhttp_bound_socket *handle);
int startHttpd(int port, char *root_path);
