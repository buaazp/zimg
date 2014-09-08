#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#ifndef WIN32
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <sys/queue.h>
#endif

#include <unistd.h>
#include <pthread.h>

#include <event2/event.h>
#include <event2/thread.h>

#include "evthr.h"

#if (__GNUC__ > 2 || ( __GNUC__ == 2 && __GNUC__MINOR__ > 4)) && (!defined(__STRICT_ANSI__) || __STRICT_ANSI__ == 0)
#define __unused__   __attribute__((unused))
#else
#define __unused__
#endif

#define _EVTHR_MAGIC 0x03fb

typedef struct evthr_cmd        evthr_cmd_t;
typedef struct evthr_pool_slist evthr_pool_slist_t;

struct evthr_cmd {
    uint8_t  stop : 1;
    void   * args;
    evthr_cb cb;
} __attribute__ ((packed));

TAILQ_HEAD(evthr_pool_slist, evthr);

struct evthr_pool {
    int                nthreads;
    evthr_pool_slist_t threads;
};

struct evthr {
    int             cur_backlog;
    int             max_backlog;
    int             rdr;
    int             wdr;
    char            err;
    ev_t          * event;
    evbase_t      * evbase;
    pthread_mutex_t lock;
    pthread_mutex_t stat_lock;
    pthread_mutex_t rlock;
    pthread_t     * thr;
    evthr_init_cb   init_cb;
    void          * arg;
    void          * aux;

    TAILQ_ENTRY(evthr) next;
};

#ifndef TAILQ_FOREACH_SAFE
#define TAILQ_FOREACH_SAFE(var, head, field, tvar)        \
    for ((var) = TAILQ_FIRST((head));                     \
         (var) && ((tvar) = TAILQ_NEXT((var), field), 1); \
         (var) = (tvar))
#endif

inline void
evthr_inc_backlog(evthr_t * evthr) {
    __sync_fetch_and_add(&evthr->cur_backlog, 1);
}

inline void
evthr_dec_backlog(evthr_t * evthr) {
    __sync_fetch_and_sub(&evthr->cur_backlog, 1);
}

inline int
evthr_get_backlog(evthr_t * evthr) {
    return __sync_add_and_fetch(&evthr->cur_backlog, 0);
}

inline void
evthr_set_max_backlog(evthr_t * evthr, int max) {
    evthr->max_backlog = max;
}

inline int
evthr_set_backlog(evthr_t * evthr, int num) {
    int rnum;

    if (evthr->wdr < 0) {
        return -1;
    }

    rnum = num * sizeof(evthr_cmd_t);

    return setsockopt(evthr->wdr, SOL_SOCKET, SO_RCVBUF, &rnum, sizeof(int));
}

static void
_evthr_read_cmd(evutil_socket_t sock, short __unused__ which, void * args) {
    evthr_t   * thread;
    evthr_cmd_t cmd;
    ssize_t     recvd;

    if (!(thread = (evthr_t *)args)) {
        return;
    }

    if (pthread_mutex_trylock(&thread->lock) != 0) {
        return;
    }

    pthread_mutex_lock(&thread->rlock);

    if ((recvd = recv(sock, &cmd, sizeof(evthr_cmd_t), 0)) <= 0) {
        pthread_mutex_unlock(&thread->rlock);
        if (errno == EAGAIN) {
            goto end;
        } else {
            goto error;
        }
    }

    if (recvd < (ssize_t)sizeof(evthr_cmd_t)) {
        pthread_mutex_unlock(&thread->rlock);
        goto error;
    }

    pthread_mutex_unlock(&thread->rlock);

    if (recvd != sizeof(evthr_cmd_t)) {
        goto error;
    }

    if (cmd.stop == 1) {
        goto stop;
    }

    if (cmd.cb != NULL) {
        cmd.cb(thread, cmd.args, thread->arg);
        goto done;
    } else {
        goto done;
    }

stop:
    event_base_loopbreak(thread->evbase);
done:
    evthr_dec_backlog(thread);
end:
    pthread_mutex_unlock(&thread->lock);
    return;
error:
    pthread_mutex_lock(&thread->stat_lock);
    thread->cur_backlog = -1;
    thread->err         = 1;
    pthread_mutex_unlock(&thread->stat_lock);
    pthread_mutex_unlock(&thread->lock);
    event_base_loopbreak(thread->evbase);
    return;
} /* _evthr_read_cmd */

static void *
_evthr_loop(void * args) {
    evthr_t * thread;

    if (!(thread = (evthr_t *)args)) {
        return NULL;
    }

    if (thread == NULL || thread->thr == NULL) {
        pthread_exit(NULL);
    }

    thread->evbase = event_base_new();
    thread->event  = event_new(thread->evbase, thread->rdr,
                               EV_READ | EV_PERSIST, _evthr_read_cmd, args);

    event_add(thread->event, NULL);

    pthread_mutex_lock(&thread->lock);
    if (thread->init_cb != NULL) {
        thread->init_cb(thread, thread->arg);
    }
    pthread_mutex_unlock(&thread->lock);

    event_base_loop(thread->evbase, 0);

    if (thread->err == 1) {
        fprintf(stderr, "FATAL ERROR!\n");
    }

    pthread_exit(NULL);
}

evthr_res
evthr_defer(evthr_t * thread, evthr_cb cb, void * arg) {
    int         cur_backlog;
    evthr_cmd_t cmd;

    cur_backlog = evthr_get_backlog(thread);

    if (thread->max_backlog) {
        if (cur_backlog + 1 > thread->max_backlog) {
            return EVTHR_RES_BACKLOG;
        }
    }

    if (cur_backlog == -1) {
        return EVTHR_RES_FATAL;
    }

    /* cmd.magic = _EVTHR_MAGIC; */
    cmd.cb   = cb;
    cmd.args = arg;
    cmd.stop = 0;

    pthread_mutex_lock(&thread->rlock);

    evthr_inc_backlog(thread);

    if (send(thread->wdr, &cmd, sizeof(cmd), 0) <= 0) {
        evthr_dec_backlog(thread);
        pthread_mutex_unlock(&thread->rlock);
        return EVTHR_RES_RETRY;
    }

    pthread_mutex_unlock(&thread->rlock);

    return EVTHR_RES_OK;
}

evthr_res
evthr_stop(evthr_t * thread) {
    evthr_cmd_t cmd;

    /* cmd.magic = _EVTHR_MAGIC; */
    cmd.cb   = NULL;
    cmd.args = NULL;
    cmd.stop = 1;

    pthread_mutex_lock(&thread->rlock);

    if (write(thread->wdr, &cmd, sizeof(evthr_cmd_t)) < 0) {
        pthread_mutex_unlock(&thread->rlock);
        return EVTHR_RES_RETRY;
    }

    pthread_mutex_unlock(&thread->rlock);

    return EVTHR_RES_OK;
}

evbase_t *
evthr_get_base(evthr_t * thr) {
    return thr->evbase;
}

void
evthr_set_aux(evthr_t * thr, void * aux) {
    thr->aux = aux;
}

void *
evthr_get_aux(evthr_t * thr) {
    return thr->aux;
}

evthr_t *
evthr_new(evthr_init_cb init_cb, void * args) {
    evthr_t * thread;
    int       fds[2];

    if (evutil_socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1) {
        return NULL;
    }

    evutil_make_socket_nonblocking(fds[0]);
    evutil_make_socket_nonblocking(fds[1]);

    if (!(thread = calloc(sizeof(evthr_t), 1))) {
        return NULL;
    }

    thread->thr     = malloc(sizeof(pthread_t));
    thread->init_cb = init_cb;
    thread->arg     = args;
    thread->rdr     = fds[0];
    thread->wdr     = fds[1];

    if (pthread_mutex_init(&thread->lock, NULL)) {
        evthr_free(thread);
        return NULL;
    }

    if (pthread_mutex_init(&thread->stat_lock, NULL)) {
        evthr_free(thread);
        return NULL;
    }

    if (pthread_mutex_init(&thread->rlock, NULL)) {
        evthr_free(thread);
        return NULL;
    }

    return thread;
} /* evthr_new */

int
evthr_start(evthr_t * thread) {
    int res;

    if (thread == NULL || thread->thr == NULL) {
        return -1;
    }

    pthread_key_create(&thread_key, NULL);

    if (pthread_create(thread->thr, NULL, _evthr_loop, (void *)thread)) {
        return -1;
    }

    res = pthread_detach(*thread->thr);

    return res;
}

void
evthr_free(evthr_t * thread) {
    if (thread == NULL) {
        return;
    }

    pthread_key_delete(thread_key);

    if (thread->rdr > 0) {
        close(thread->rdr);
    }

    if (thread->wdr > 0) {
        close(thread->wdr);
    }

    if (thread->thr) {
        free(thread->thr);
    }

    if (thread->event) {
        event_free(thread->event);
    }

    if (thread->evbase) {
        event_base_free(thread->evbase);
    }

    free(thread);
} /* evthr_free */

void
evthr_pool_free(evthr_pool_t * pool) {
    evthr_t * thread;
    evthr_t * save;

    if (pool == NULL) {
        return;
    }

    TAILQ_FOREACH_SAFE(thread, &pool->threads, next, save) {
        TAILQ_REMOVE(&pool->threads, thread, next);

        evthr_free(thread);
    }

    free(pool);
}

evthr_res
evthr_pool_stop(evthr_pool_t * pool) {
    evthr_t * thr;
    evthr_t * save;

    if (pool == NULL) {
        return EVTHR_RES_FATAL;
    }

    TAILQ_FOREACH_SAFE(thr, &pool->threads, next, save) {
        evthr_stop(thr);
    }

    return EVTHR_RES_OK;
}

evthr_res
evthr_pool_defer(evthr_pool_t * pool, evthr_cb cb, void * arg) {
    evthr_t * min_thr = NULL;
    evthr_t * thr     = NULL;

    if (pool == NULL) {
        return EVTHR_RES_FATAL;
    }

    if (cb == NULL) {
        return EVTHR_RES_NOCB;
    }

    /* find the thread with the smallest backlog */
    TAILQ_FOREACH(thr, &pool->threads, next) {
        int thr_backlog = 0;
        int min_backlog = 0;

        thr_backlog = evthr_get_backlog(thr);

        if (min_thr) {
            min_backlog = evthr_get_backlog(min_thr);
        }

        if (min_thr == NULL) {
            min_thr = thr;
        } else if (thr_backlog == 0) {
            min_thr = thr;
	    break;
        } else if (thr_backlog < min_backlog) {
            min_thr = thr;
        }

    }

    return evthr_defer(min_thr, cb, arg);
} /* evthr_pool_defer */

evthr_pool_t *
evthr_pool_new(int nthreads, evthr_init_cb init_cb, void * shared) {
    evthr_pool_t * pool;
    int            i;

    if (nthreads == 0) {
        return NULL;
    }

    if (!(pool = calloc(sizeof(evthr_pool_t), 1))) {
        return NULL;
    }

    pool->nthreads = nthreads;
    TAILQ_INIT(&pool->threads);

    for (i = 0; i < nthreads; i++) {
        evthr_t * thread;

        if (!(thread = evthr_new(init_cb, shared))) {
            evthr_pool_free(pool);
            return NULL;
        }

        TAILQ_INSERT_TAIL(&pool->threads, thread, next);
    }

    return pool;
}

int
evthr_pool_set_backlog(evthr_pool_t * pool, int num) {
    evthr_t * thr;

    TAILQ_FOREACH(thr, &pool->threads, next) {
        evthr_set_backlog(thr, num);
    }

    return 0;
}

void
evthr_pool_set_max_backlog(evthr_pool_t * pool, int max) {
    evthr_t * thr;

    TAILQ_FOREACH(thr, &pool->threads, next) {
        evthr_set_max_backlog(thr, max);
    }
}

int
evthr_pool_start(evthr_pool_t * pool) {
    evthr_t * evthr = NULL;

    if (pool == NULL) {
        return -1;
    }

    TAILQ_FOREACH(evthr, &pool->threads, next) {
        if (evthr_start(evthr) < 0) {
            return -1;
        }

        usleep(5000);
    }

    return 0;
}

