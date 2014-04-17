#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include <inttypes.h>

#include "htparse.h"

#define ADD_DATA_BUF(buf, name, data, len) do { \
        strcat(buf, name ": '");                \
        strncat(buf, data, len);                \
        strcat(buf, "'\n");                     \
} while (0)

struct testobj {
    char   * name;
    char   * data;
    htp_type type;
};


static int
_msg_begin(htparser * p) {
    char * buf = (char *)htparser_get_userdata(p);

    strcat(buf, "START\n");

    return 0;
}

static int
_method(htparser * p, const char * b, size_t s) {
    char * buf = (char *)htparser_get_userdata(p);

    if (htparser_get_method(p) == htp_method_UNKNOWN) {
        ADD_DATA_BUF(buf, "METHOD_UNKNOWN", b, s);
        return htp_method_UNKNOWN;
    } else {
        ADD_DATA_BUF(buf, "METHOD", b, s);
    }

    return 0;
}

static int
_scheme(htparser * p, const char * b, size_t s) {
    char * buf = (char *)htparser_get_userdata(p);

    ADD_DATA_BUF(buf, "SCHEME", b, s);

    return 0;
}

static int
_host(htparser * p, const char * b, size_t s) {
    char * buf = (char *)htparser_get_userdata(p);

    ADD_DATA_BUF(buf, "HOST", b, s);

    return 0;
}

static int
_port(htparser * p, const char * b, size_t s) {
    char * buf = (char *)htparser_get_userdata(p);

    ADD_DATA_BUF(buf, "PORT", b, s);

    return 0;
}

static int
_path(htparser * p, const char * b, size_t s) {
    char * buf = (char *)htparser_get_userdata(p);

    ADD_DATA_BUF(buf, "PATH", b, s);

    return 0;
}

static int
_args(htparser * p, const char * b, size_t s) {
    char * buf = (char *)htparser_get_userdata(p);

    ADD_DATA_BUF(buf, "ARGS", b, s);

    return 0;
}

static int
_uri(htparser * p, const char * b, size_t s) {
    char * buf = (char *)htparser_get_userdata(p);

    ADD_DATA_BUF(buf, "URI", b, s);

    return 0;
}

static int
_hdrs_begin(htparser * p) {
    char * buf = (char *)htparser_get_userdata(p);

    strcat(buf, "HDRS_BEGIN\n");

    return 0;
}

static int
_hdr_key(htparser * p, const char * b, size_t s) {
    char * buf = (char *)htparser_get_userdata(p);

    ADD_DATA_BUF(buf, "HDR_KEY", b, s);

    return 0;
}

static int
_hdr_val(htparser * p, const char * b, size_t s) {
    char * buf = (char *)htparser_get_userdata(p);

    ADD_DATA_BUF(buf, "HDR_VAL", b, s);

    return 0;
}

static int
_hostname(htparser * p, const char * b, size_t s) {
    char * buf = (char *)htparser_get_userdata(p);

    ADD_DATA_BUF(buf, "HOSTNAME", b, s);

    return 0;
}

static int
_hdrs_complete(htparser * p) {
    char * buf = (char *)htparser_get_userdata(p);

    strcat(buf, "HDRS_COMPLETE\n");

    return 0;
}

static int
_new_chunk(htparser * p) {
    char * buf        = (char *)htparser_get_userdata(p);
    char   tbuf[1024] = { 0 };

    sprintf(tbuf, "NEW_CHUNK: %" PRIu64 "\n", htparser_get_content_length(p));
    strcat(buf, tbuf);

    return 0;
}

static int
_chunk_complete(htparser * p) {
    char * buf = (char *)htparser_get_userdata(p);

    strcat(buf, "END_CHUNK\n");

    return 0;
}

static int
_chunks_complete(htparser * p) {
    char * buf = (char *)htparser_get_userdata(p);

    strcat(buf, "END_CHUNKS\n");

    return 0;
}

static int
_body(htparser * p, const char * b, size_t s) {
    char * buf = (char *)htparser_get_userdata(p);

    ADD_DATA_BUF(buf, "BODY", b, s);

    return 0;
}

static int
_msg_complete(htparser * p) {
    char * buf = (char *)htparser_get_userdata(p);

    strcat(buf, "MSG_COMPLETE\n");

    return 0;
}

htparse_hooks hooks = {
    .on_msg_begin       = _msg_begin,
    .method             = _method,
    .scheme             = _scheme,
    .host               = _host,
    .port               = _port,
    .path               = _path,
    .args               = _args,
    .uri                = _uri,
    .on_hdrs_begin      = _hdrs_begin,
    .hdr_key            = _hdr_key,
    .hdr_val            = _hdr_val,
    .hostname           = _hostname,
    .on_hdrs_complete   = _hdrs_complete,
    .on_new_chunk       = _new_chunk,
    .on_chunk_complete  = _chunk_complete,
    .on_chunks_complete = _chunks_complete,
    .body               = _body,
    .on_msg_complete    = _msg_complete
};


struct testobj t1 = {
    .name = "small GET request",
    .type = htp_type_request,
    .data = "GET / HTTP/1.0\r\n\r\n"
};

struct testobj t2 = {
    .name = "GET request with arguments",
    .type = htp_type_request,
    .data = "GET /test?a=b&c=d HTTP/1.1\r\n\r\n"
};

struct testobj t3 = {
    .name = "POST request with 4 bytes of data",
    .type = htp_type_request,
    .data = "POST /foo/bar HTTP/1.0\r\n"
            "Content-Length: 4\r\n\r\n"
            "abcd"
};

struct testobj t4 = {
    .name = "Simple POST with chunked data",
    .type = htp_type_request,
    .data = "POST /test/ HTTP/1.1\r\n"
            "Transfer-Encoding: chunked\r\n\r\n"
            "1e\r\nall your base are belong to us\r\n"
            "0\r\n"
            "\r\n"
};

struct testobj t5 = {
    .name = "POST request with multiple chunks",
    .type = htp_type_request,
    .data = "POST /test/ HTTP/1.1\r\n"
	    "Connection: Keep-Alive\r\n"
            "Transfer-Encoding: chunked\r\n\r\n"
            "23\r\n"
            "This is the data in the first chunk"
            "\r\n"
            "1A\r\n"
            "and this is the second one"
            "\r\n"
            "3\r\n"
            "foo\r\n"
            "6\r\n"
            "barbaz\r\n"
            "0\r\n\r\n"
};

struct testobj t6 = {
    .name = "GET request with a host header",
    .type = htp_type_request,
    .data = "GET /test/ HTTP/1.0\r\n"
            "Host: ieatfood.net\r\n\r\n"
};

struct testobj t7 = {
    .name = "GET request with an empty header value",
    .type = htp_type_request,
    .data = "GET /test/ HTTP/1.0\r\n"
            "Header1: value1\r\n"
            "Header2: \r\n"
            "Header3: value3\r\n\r\n"
};

struct testobj t8 = {
    .name = "GET request with a multi-line header value",
    .type = htp_type_request,
    .data = "GET /test/ HTTP/1.1\r\n"
            "Header1: value1\r\n"
            "Header2: val\r\n"
            "\tue\r\n"
            "\t2\r\n"
            "Header3: value3\r\n\r\n"
};

struct testobj t9 = {
    .name = "[FAILURE TEST] GET REQUEST with LF instead of CRLF on header value",
    .type = htp_type_request,
    .data = "GET /test/ HTTP/1.1\r\n"
            "Header: value\n\n"
};

struct testobj t10 = {
    .name = "[FAILURE TEST] GET request with invalid protocol",
    .type = htp_type_request,
    .data = "GET /test/ fdasfs\r\n\r\n"
};

struct testobj t11 = {
    .name = "[FALURE TEST] POST request with invalid chunk length",
    .type = htp_type_request,
    .data = "POST /test/ HTTP/1.1\r\n"
            "Transfer-Encoding: chunked\r\n\r\n"
            "3\r\n"
            "foo"
            "\r\n"
            "A\r\n"
            "foobar\r\n"
            "3\r\n"
            "baz\r\n"
            "0\r\n\r\n"
};

struct testobj t12 = {
    .name = "Simple GET on a FTP scheme",
    .type = htp_type_request,
    .data = "GET ftp://test.com/foo/bar HTTP/1.1\r\n\r\n"
};

struct testobj t13 = {
    .name = "Multiple GET requests in HTTP/1.1 request",
    .type = htp_type_request,
    .data = "GET /request1 HTTP/1.1\r\n\r\n"
            "GET /request2 HTTP/1.0\r\n"
            "Connection: close\r\n\r\n"
};

struct testobj t14 = {
    .name = "[FAILURE TEST] invalid request type",
    .type = htp_type_request,
    .data = "DERP /test HTTP/1.1\r\n\r\n"
};

struct testobj t15 = {
    .name = "http SCHEME request with port / args / headers",
    .type = htp_type_request,
    .data = "GET http://ieatfood.net:80/index.html?foo=bar&baz=buz HTTP/1.0\r\n"
            "Host: ieatfood.net\r\n"
            "Header: value\r\n\r\n"
};

struct testobj t16 = {
    .name = "GET request which should run all callbacks minus scheme stuff, this includes multiple requests",
    .type = htp_type_request,
    .data = "GET /test1?a=b&c=d&e=f HTTP/1.1\r\n"
            "Content-Length: 6\r\n\r\n"
            "foobar"
            "GET /test2 HTTP/1.1\r\n"
            "Header: test2\r\n\r\n"
            "POST /test/ HTTP/1.1\r\n"
            "Transfer-Encoding: chunked\r\n\r\n"
            "23\r\n"
            "This is the data in the first chunk"
            "\r\n"
            "1A\r\n"
            "and this is the second one"
            "\r\n"
            "3\r\n"
            "foo\r\n"
            "6\r\n"
            "barbaz\r\n"
            "0\r\n\r\n"
            "GET /test/ HTTP/1.1\r\n"
            "Host: ieatfood.net\r\n"
            "Connection: close\r\n\r\n"
};

struct testobj t17 = {
    .name = "Like the last test, but with scheme requests",
    .type = htp_type_request,
    .data = "GET http://ieatfood.net/test1?a=b&c=d&e=f HTTP/1.1\r\n"
            "Content-Length: 6\r\n\r\n"
            "foobar"
            "GET https://ieatfood.net:443/test2 HTTP/1.1\r\n"
            "Header: test2\r\n\r\n"
            "POST /test/ HTTP/1.1\r\n"
            "Transfer-Encoding: chunked\r\n\r\n"
            "23\r\n"
            "This is the data in the first chunk"
            "\r\n"
            "1A\r\n"
            "and this is the second one"
            "\r\n"
            "3\r\n"
            "foo\r\n"
            "6\r\n"
            "barbaz\r\n"
            "0\r\n\r\n"
            "GET ftp://ackers.net:21/test/ HTTP/1.1\r\n"
            "Host: ackers.net\r\n"
            "Connection: close\r\n\r\n"
};

struct testobj t18 = {
    .name = "scheme request with empty path",
    .type = htp_type_request,
    .data = "GET http://ackers.net HTTP/1.0\r\n\r\n"
};

struct testobj t19 = {
    .name = "basic HTTP RESPONSE",
    .type = htp_type_response,
    .data = "HTTP/1.1 200 OK\r\n\r\n"
};

struct testobj t20 = {
    .name = "HTTP RESPONSE with body",
    .type = htp_type_response,
    .data = "HTTP/1.1 200 OK\r\n"
            "Content-Length: 6\r\n\r\n"
            "foobar"
};

struct testobj t21 = {
    .name = "HTTP RESPONSE with chunked data",
    .type = htp_type_response,
    .data = "HTTP/1.1 200 OK\r\n"
            "Transfer-Encoding: chunked\r\n\r\n"
            "23\r\n"
            "This is the data in the first chunk"
            "\r\n"
            "1A\r\n"
            "and this is the second one"
            "\r\n"
            "3\r\n"
            "foo\r\n"
            "6\r\n"
            "barbaz\r\n"
            "0\r\n\r\n"
};

struct testobj t22 = {
    .name = "Header key with no value",
    .type = htp_type_request,
    .data = "GET / HTTP/1.1\r\n"
            "Accept\r\n\r\n"
};


static int
_run_test(htparser * p, struct testobj * obj) {
    size_t data_sz;
    size_t parsed_sz;
    char   result_buf[5000] = { 0 };

    htparser_init(p, obj->type);
    htparser_set_userdata(p, result_buf);

    data_sz   = strlen(obj->data);
    parsed_sz = htparser_run(p, &hooks, obj->data, data_sz);

    strcat(result_buf, "ERROR_STR: ");
    strcat(result_buf, htparser_get_strerror(p));
    strcat(result_buf, "\n");

    printf("%s\n", obj->name);
    printf("-----------------\n");
    printf("%s", result_buf);
    printf("\n");

    return 0;
}

int
main(int argc, char **argv) {
    htparser * parser;

    parser = htparser_new();
    assert(parser != NULL);

    _run_test(parser, &t1);
    _run_test(parser, &t2);
    _run_test(parser, &t3);
    _run_test(parser, &t4);
    _run_test(parser, &t5);
    _run_test(parser, &t6);
    _run_test(parser, &t7);
    _run_test(parser, &t8);
    _run_test(parser, &t9);
    _run_test(parser, &t10);
    _run_test(parser, &t11);
    _run_test(parser, &t12);
    _run_test(parser, &t13);
    _run_test(parser, &t14);
    _run_test(parser, &t15);
    _run_test(parser, &t16);
    _run_test(parser, &t17);
    _run_test(parser, &t18);
    _run_test(parser, &t19);
    _run_test(parser, &t20);
    _run_test(parser, &t21);
    _run_test(parser, &t22);

    return 0;
}

