#ifndef ZHTTPD_H
#define ZHTTPD_H

#include "zcommon.h"

static const char * guess_content_type(const char *path);
//void kmp_init(const unsigned char *pattern, int pattern_size);
//int kmp(const unsigned char *matcher, int mlen, const unsigned char *pattern, int plen);
int find_cache(const char *key, char *value);
int set_cache(const char *key, const char *value);
int del_cache(const char *key);
void dump_request_cb(struct evhttp_request *req, void *arg);
void post_request_cb(struct evhttp_request *req, void *arg);
void send_document_cb(struct evhttp_request *req, void *arg);
static int display_address(struct evhttp_bound_socket *handle);
int start_httpd(int port);

#endif
