/*   
 *   zimg - high performance image storage and processing system.
 *       http://zimg.buaa.us 
 *   
 *   Copyright (c) 2013, Peter Zhao <zp@buaa.us>.
 *   All rights reserved.
 *   
 *   Use and distribution licensed under the BSD license.
 *   See the LICENSE file for full text.
 * 
 */


/**
 * @file zworkqueue.h
 * @brief Workqueue header.
 * @author 招牌疯子 zp@buaa.us
 * @version 1.0
 * @date 2013-07-19
 */

#ifndef ZWORKQUEUE_H
#define ZWORKQUEUE_H

#include <pthread.h>
#include <event.h>
#include <evhttp.h>

typedef struct worker {
	pthread_t thread;
    struct event_base *evbase;
    struct evhttp *http;
	int terminate;
	struct workqueue *workqueue;
	struct worker *prev;
	struct worker *next;
} worker_t;

typedef struct job {
	int user_data;
	struct job *prev;
	struct job *next;
} job_t;

typedef struct workqueue {
	struct worker *workers;
	struct job *waiting_jobs;
	pthread_mutex_t jobs_mutex;
	pthread_cond_t jobs_cond;
} workqueue_t;

int workqueue_init(workqueue_t *workqueue, int numWorkers);

void workqueue_shutdown(workqueue_t *workqueue);

void workqueue_add_job(workqueue_t *workqueue, job_t *job);

#endif	/* #ifndef WORKQUEUE_H */
