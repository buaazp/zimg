#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#ifndef __EVTHR_H__
#define __EVTHR_H__

#include <sched.h>
#include <pthread.h>
#include <sys/queue.h>
#include <event2/event.h>
#include <event2/thread.h>

#ifdef __cplusplus
extern "C" {
#endif

enum evthr_res {
    EVTHR_RES_OK = 0,
    EVTHR_RES_BACKLOG,
    EVTHR_RES_RETRY,
    EVTHR_RES_NOCB,
    EVTHR_RES_FATAL
};

pthread_key_t thread_key;

struct evthr_pool;
struct evthr;

typedef struct event_base evbase_t;
typedef struct event      ev_t;

typedef struct evthr_pool evthr_pool_t;
typedef struct evthr      evthr_t;
typedef enum evthr_res    evthr_res;

typedef void (*evthr_cb)(evthr_t * thr, void * cmd_arg, void * shared);
typedef void (*evthr_init_cb)(evthr_t * thr, void * shared);

evthr_t      * evthr_new(evthr_init_cb init_cb, void * arg);
evbase_t     * evthr_get_base(evthr_t * thr);
void           evthr_set_aux(evthr_t * thr, void * aux);
void         * evthr_get_aux(evthr_t * thr);
int            evthr_start(evthr_t * evthr);
evthr_res      evthr_stop(evthr_t * evthr);
evthr_res      evthr_defer(evthr_t * evthr, evthr_cb cb, void * arg);
void           evthr_free(evthr_t * evthr);
void           evthr_inc_backlog(evthr_t * evthr);
void           evthr_dec_backlog(evthr_t * evthr);
int            evthr_get_backlog(evthr_t * evthr);
void           evthr_set_max_backlog(evthr_t * evthr, int max);
int            evthr_set_backlog(evthr_t *, int);

evthr_pool_t * evthr_pool_new(int nthreads, evthr_init_cb init_cb, void * shared);
int            evthr_pool_start(evthr_pool_t * pool);
evthr_res      evthr_pool_stop(evthr_pool_t * pool);
evthr_res      evthr_pool_defer(evthr_pool_t * pool, evthr_cb cb, void * arg);
void           evthr_pool_free(evthr_pool_t * pool);
void           evthr_pool_set_max_backlog(evthr_pool_t * evthr, int max);
int            evthr_pool_set_backlog(evthr_pool_t *, int);

#ifdef __cplusplus
}
#endif

#endif /* __EVTHR_H__ */

