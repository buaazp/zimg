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
 * @file zworkqueue.c
 * @brief Workqueue functions used for multi-thread libevent support.
 * @author 招牌疯子 zp@buaa.us
 * @version 1.0
 * @date 2013-07-19
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zworkqueue.h"
#include "zhttpd.h"
#include "zcommon.h"

static void *worker_function(void *ptr);
int workqueue_init(workqueue_t *workqueue, int numWorkers);
void workqueue_shutdown(workqueue_t *workqueue);
void workqueue_add_job(workqueue_t *workqueue, job_t *job); 


#define LL_ADD(item, list) { \
	if(list != NULL) \
    { \
        item->next = list; \
        item->next->prev = item; \
        item->prev = NULL; \
    } \
    else \
    { \
        item->next = NULL; \
        item->prev = NULL; \
    } \
	list = item; \
}

#define LL_REMOVE(item, list) { \
	if (item->prev != NULL) item->prev->next = item->next; \
	if (item->next != NULL) item->next->prev = item->prev; \
	if (list == item) list = item->next; \
	item->prev = item->next = NULL; \
}

static void *worker_function(void *ptr) 
{
	worker_t *worker = (worker_t *)ptr;
	job_t *job;
    LOG_PRINT(LOG_INFO, "Worker Created. Waiting for Jobs... ");

	while (1) {
		/* Wait until we get notified. */
		pthread_mutex_lock(&worker->workqueue->jobs_mutex);
		while (worker->workqueue->waiting_jobs == NULL) {
			pthread_cond_wait(&worker->workqueue->jobs_cond, &worker->workqueue->jobs_mutex);
            /* If we're supposed to terminate, break out of our continuous loop. */
            if (worker->terminate) break;
		}
		job = worker->workqueue->waiting_jobs;
		if (job != NULL) {
			LL_REMOVE(job, worker->workqueue->waiting_jobs);
		}
		pthread_mutex_unlock(&worker->workqueue->jobs_mutex);

        /* If we're supposed to terminate, break out of our continuous loop. */
        if (worker->terminate) break;

		/* If we didn't get a job, then there's nothing to do at this time. */
		if (job == NULL) continue;

		/* Execute the job. */
		//job->job_function(job);
        struct evhttp_bound_socket *handle;
        handle = evhttp_accept_socket_with_handle(worker->http, job->user_data);
        if (handle != NULL) {
            LOG_PRINT(LOG_INFO, "Bound to port - Awaiting connections ... ");
            //mm_free(handle);
        }
        free(job);
        event_base_dispatch(worker->evbase);
	}

    LOG_PRINT(LOG_INFO, "Stopping worker_evhttp loop.");
    if (event_base_loopexit(worker->evbase, NULL)) {
        LOG_PRINT(LOG_ERROR, "Error shutting down server");
    }
    if (worker->http != NULL) {
        evhttp_free(worker->http);
        worker->http = NULL;
    }
    if (worker->evbase != NULL) {
        event_base_free(worker->evbase);
        worker->evbase = NULL;
    }
    LL_REMOVE(worker, worker->workqueue->workers);
    LOG_PRINT(LOG_INFO, "Worker Shutdown.");
	free(worker);
	pthread_exit(NULL);
}

int workqueue_init(workqueue_t *workqueue, int numWorkers) 
{
	int i;
	worker_t *worker;
	pthread_cond_t blank_cond = PTHREAD_COND_INITIALIZER;
	pthread_mutex_t blank_mutex = PTHREAD_MUTEX_INITIALIZER;

	if (numWorkers < 1) numWorkers = 1;
	memset(workqueue, 0, sizeof(*workqueue));
	memcpy(&workqueue->jobs_mutex, &blank_mutex, sizeof(workqueue->jobs_mutex));
	memcpy(&workqueue->jobs_cond, &blank_cond, sizeof(workqueue->jobs_cond));

	for (i = 0; i < numWorkers; i++) {
		if ((worker = malloc(sizeof(worker_t))) == NULL) {
			perror("Failed to allocate all workers");
			return 1;
		}
		memset(worker, 0, sizeof(*worker));
        
        if ((worker->evbase = event_base_new()) == NULL) {
            LOG_PRINT(LOG_WARNING, "Worker event_base creation failed");
            return 1;
        }

        if ((worker->http = evhttp_new(worker->evbase)) == NULL) {
            LOG_PRINT(LOG_WARNING, "Worker evhttp creation failed");
            return 1;
        }

        evhttp_set_cb(worker->http, "/dump", dump_request_cb, worker);
        evhttp_set_cb(worker->http, "/upload", post_request_cb, worker);
        evhttp_set_gencb(worker->http, send_document_cb, worker);

        worker->workqueue = workqueue;
		if (pthread_create(&worker->thread, NULL, worker_function, (void *)worker)) {
			perror("Failed to start all worker threads");
			free(worker);
			return 1;
		}
		LL_ADD(worker, worker->workqueue->workers);
	}

	return 0;
}

void workqueue_shutdown(workqueue_t *workqueue) 
{
    LOG_PRINT(LOG_INFO, "Start to Close Workers...");
	worker_t *worker = NULL;

	/* Set all workers to terminate. */
	for (worker = workqueue->workers; worker != NULL; worker = worker->next) 
    {
		worker->terminate = 1;
	}

	/* Remove all workers and jobs from the work queue.
	 * wake up all workers so that they will terminate. */
	pthread_mutex_lock(&workqueue->jobs_mutex);
    LOG_PRINT(LOG_INFO, "broadcast shutdown signal...");
	//workqueue->workers = NULL;
	workqueue->waiting_jobs = NULL;
	pthread_cond_broadcast(&workqueue->jobs_cond);
	pthread_mutex_unlock(&workqueue->jobs_mutex);
}

void workqueue_add_job(workqueue_t *workqueue, job_t *job) 
{
	/* Add the job to the job queue, and notify a worker. */
	pthread_mutex_lock(&workqueue->jobs_mutex);
	LL_ADD(job, workqueue->waiting_jobs);
	pthread_cond_signal(&workqueue->jobs_cond);
	pthread_mutex_unlock(&workqueue->jobs_mutex);
}
