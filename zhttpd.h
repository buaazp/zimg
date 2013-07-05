#include "zcommon.h"

static const char * guess_content_type(const char *path);
//void kmp_init(const unsigned char *pattern, int pattern_size);
//int kmp(const unsigned char *matcher, int mlen, const unsigned char *pattern, int plen);
int find_cache(const char *key, char *value);
int set_cache(const char *key, const char *value);
int del_cache(const char *key);
static void dump_request_cb(struct evhttp_request *req, void *arg);
static void post_request_cb(struct evhttp_request *req, void *arg);
static void zimg_cb(struct evhttp_request *req, void *arg);
static void send_document_cb(struct evhttp_request *req, void *arg);
static int displayAddress(struct evhttp_bound_socket *handle);
int startHttpd(int port, char *root_path);
