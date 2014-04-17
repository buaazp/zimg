/*
 * How to exploit the wonders of libevhtp's threading model to avoid using
 * libevent's locking API.
 *
 * In this example we use Redis's Async API (Libhiredis) store and retr the following
 * information for a request:
 *
 * Total requests seen.
 * Total requests seen by the requestors IP address.
 * All of the source ports seen used by the requestors IP address.
 *
 * We do this all using libevhtp's builtin thread-pool model, without the use of
 * mutexes or evthread_use_pthreads() type stuff.
 *
 * The technique is simple:
 *  1. Create your evhtp_t structure, assign callbacks like usual.
 *  2. Call evhtp_use_threads() with a thread init callback.
 *  3. Each time a thread starts, the thread init callback you defined will be
 *     called with information about that thread.
 *
 *     First a bit of information about how evhtp does threading:
 *       libevhtp uses the evthr library, which works more like a threaded
 *       co-routine than a threadpool. Each evthr in a pool has its own unique
 *       event_base (and each evthr runs its own event_base_loop()). Under the
 *       hood when libevhtp sends a request to a thread, it calls
 *       "evthr_pool_defer(pool, _run_connection_in_thread, ...).
 *
 *       The evthr library then finds a thread inside the pool with the lowest backlog,
 *       sends a packet over that threads socketpair containing information about what
 *       function to execute. It uses socketpairs because they can be treated as
 *       an event, thus able to be processed in a threads own unique
 *       event_base_loop().
 *
 *       Knowing that, a connection in evhtp is never associated with the initial
 *       event_base that was passed to evhtp_new(), but instead the connection
 *       uses the evthr's unique event_base. This is what makes libevhtp's
 *       safe from thread-related race conditions.
 *
 *  4. Use the thread init callback as a place to put event type things on the
 *     threads event_base() instead of using the global one.
 *
 *     In this code, that function is app_init_thread(). When this function is
 *     called, the first argument is the evthr_t of the thread that just
 *     started. This function uses "evthr_get_base(thread)" to get the
 *     event_base associated with this specific thread.
 *
 *     Using that event_base, the function will start up an async redis
 *     connection. This redis connection is now tied to that thread, and can be
 *     used on a threaded request without locking (remember that your request
 *     has the same event_base as the thread it was executed in).
 *
 *     We allocate a dummy structure "struct app" and then call
 *     "evthr_set_aux(thread, app)". This function sets some aux data which can
 *     be fetched at any point using evthr_get_aux(thread). We use this later on
 *     inside process_request()
 *
 *     This part is the secret to evhtp threading success.
 *
 *  5. When a request has been fully processed, it will call the function
 *     "app_process_request()". Note here that the "arg" argument is NULL since no
 *      arguments were passed to evhtp_set_gencb().
 *
 *     Since we want to do a bunch of redis stuff before sending a reply to the
 *     client, we must fetch the "struct app" data we allocated and set for the
 *     thread associated with this request (struct app * app =
 *     evthr_get_aux(thread);).
 *
 *     struct app has our thread-specific redis connection ctx, so using that
 *     redisAsyncCommand() is called a bunch of times to queue up the commands
 *     which will be run.
 *
 *     The last part of this technique is to call the function
 *     "evhtp_request_pause()". This essentially tells evhtp to flip the
 *     read-side of the connections file-descriptor OFF (This avoids potential
 *     situations where a client disconnected before all of the redis commands
 *     executed).
 *
 *  6. Each redis command is executed in order, and each callback will write to
 *     the requests output_buffer with relevant information from the result.
 *
 *  7. The last redis callback executed here is "redis_get_srcport_cb". It is
 *      the job os this function to call evhtp_send_reply() and then
 *      evhtp_request_resume().
 *
 * Using this design in conjunction with libevhtp makes the world an easier
 * place to code.
 *
 * Compile: gcc thread_design.c -o thread_design -levhtp -levent -lhiredis
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include <evhtp.h>
#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/adapters/libevent.h>

struct app_parent {
    evhtp_t  * evhtp;
    evbase_t * evbase;
    char     * redis_host;
    uint16_t   redis_port;
};

struct app {
    struct app_parent * parent;
    evbase_t          * evbase;
    redisAsyncContext * redis;
};

static evthr_t *
get_request_thr(evhtp_request_t * request) {
    evhtp_connection_t * htpconn;
    evthr_t            * thread;

    htpconn = evhtp_request_get_connection(request);
    thread  = htpconn->thread;

    return thread;
}

void
redis_global_incr_cb(redisAsyncContext * redis, void * redis_reply, void * arg) {
    redisReply      * reply   = redis_reply;
    evhtp_request_t * request = arg;

    printf("global_incr_cb(%p)\n", request);

    if (reply == NULL || reply->type != REDIS_REPLY_INTEGER) {
        evbuffer_add_printf(request->buffer_out,
                            "redis_global_incr_cb() failed\n");
        return;
    }

    evbuffer_add_printf(request->buffer_out,
                        "Total requests = %lld\n", reply->integer);
}

void
redis_srcaddr_incr_cb(redisAsyncContext * redis, void * redis_reply, void * arg) {
    redisReply      * reply   = redis_reply;
    evhtp_request_t * request = arg;

    printf("incr_cb(%p)\n", request);

    if (reply == NULL || reply->type != REDIS_REPLY_INTEGER) {
        evbuffer_add_printf(request->buffer_out,
                            "redis_srcaddr_incr_cb() failed\n");
        return;
    }

    evbuffer_add_printf(request->buffer_out,
                        "Requests from this source IP = %lld\n", reply->integer);
}

void
redis_set_srcport_cb(redisAsyncContext * redis, void * redis_reply, void * arg) {
    redisReply      * reply   = redis_reply;
    evhtp_request_t * request = arg;

    printf("set_srcport_cb(%p)\n", request);

    if (reply == NULL || reply->type != REDIS_REPLY_INTEGER) {
        evbuffer_add_printf(request->buffer_out,
                            "redis_set_srcport_cb() failed\n");
        return;
    }

    if (!reply->integer) {
        evbuffer_add_printf(request->buffer_out,
                            "This source port has been seen already.\n");
    } else {
        evbuffer_add_printf(request->buffer_out,
                            "This source port has never been seen.\n");
    }
}

void
redis_get_srcport_cb(redisAsyncContext * redis, void * redis_reply, void * arg) {
    redisReply      * reply   = redis_reply;
    evhtp_request_t * request = arg;
    int               i;

    printf("get_srcport_cb(%p)\n", request);

    if (reply == NULL || reply->type != REDIS_REPLY_ARRAY) {
        evbuffer_add_printf(request->buffer_out,
                            "redis_get_srcport_cb() failed.\n");
        return;
    }

    evbuffer_add_printf(request->buffer_out,
                        "source ports which have been seen for your ip:\n");

    for (i = 0; i < reply->elements; i++) {
        redisReply * elem = reply->element[i];

        evbuffer_add_printf(request->buffer_out, "%s ", elem->str);
    }

    evbuffer_add(request->buffer_out, "\n", 1);

    /* final callback for redis, so send the response */
    evhtp_send_reply(request, EVHTP_RES_OK);
    evhtp_request_resume(request);
}

void
app_process_request(evhtp_request_t * request, void * arg) {
    struct sockaddr_in * sin;
    struct app_parent  * app_parent;
    struct app         * app;
    evthr_t            * thread;
    evhtp_connection_t * conn;
    char                 tmp[1024];

    printf("process_request(%p)\n", request);

    thread = get_request_thr(request);
    conn   = evhtp_request_get_connection(request);
    app    = (struct app *)evthr_get_aux(thread);
    sin    = (struct sockaddr_in *)conn->saddr;

    evutil_inet_ntop(AF_INET, &sin->sin_addr, tmp, sizeof(tmp));

    /* increment a global counter of hits on redis */
    redisAsyncCommand(app->redis, redis_global_incr_cb,
                      (void *)request, "INCR requests:total");

    /* increment a counter for hits from this source address on redis */
    redisAsyncCommand(app->redis, redis_srcaddr_incr_cb,
                      (void *)request, "INCR requests:ip:%s", tmp);

    /* add the source port of this request to a source-specific set */
    redisAsyncCommand(app->redis, redis_set_srcport_cb, (void *)request,
                      "SADD requests:ip:%s:ports %d", tmp, ntohs(sin->sin_port));

    /* get all of the ports this source address has used */
    redisAsyncCommand(app->redis, redis_get_srcport_cb, (void *)request,
                      "SMEMBERS requests:ip:%s:ports", tmp);

    /* pause the request processing */
    evhtp_request_pause(request);
}

void
app_init_thread(evhtp_t * htp, evthr_t * thread, void * arg) {
    struct app_parent * app_parent;
    struct app        * app;

    app_parent  = (struct app_parent *)arg;
    app         = calloc(sizeof(struct app), 1);

    app->parent = app_parent;
    app->evbase = evthr_get_base(thread);
    app->redis  = redisAsyncConnect(app_parent->redis_host, app_parent->redis_port);

    redisLibeventAttach(app->redis, app->evbase);

    evthr_set_aux(thread, app);
}

int
main(int argc, char ** argv) {
    evbase_t          * evbase;
    evhtp_t           * evhtp;
    struct app_parent * app_p;

    evbase            = event_base_new();
    evhtp             = evhtp_new(evbase, NULL);
    app_p             = calloc(sizeof(struct app_parent), 1);

    app_p->evhtp      = evhtp;
    app_p->evbase     = evbase;
    app_p->redis_host = "127.0.0.1";
    app_p->redis_port = 6379;

    evhtp_set_gencb(evhtp, app_process_request, NULL);
    evhtp_use_threads(evhtp, app_init_thread, 4, app_p);
    evhtp_bind_socket(evhtp, "127.0.0.1", 9090, 1024);

    event_base_loop(evbase, 0);

    return 0;
}

