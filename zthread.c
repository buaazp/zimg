/**
 * Multithreaded, libevent-based socket server.
 * Copyright (c) 2012 Ronald Bennett Cemer
 * This software is licensed under the BSD license.
 * See the accompanying LICENSE.txt for details.
 *
 * To compile: gcc -o echoserver_threaded echoserver_threaded.c workqueue.c -levent -lpthread
 * To run: ./echoserver_threaded
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include <event.h>
#include <signal.h>
#include "zworkqueue.h"
#include "zcommon.h"

/* Connection backlog (# of backlogged connections to accept). */
#define CONNECTION_BACKLOG 128
/* Number of worker threads.  Should match number of CPU cores reported in /proc/cpuinfo. */
#define NUM_THREADS 4

/**
 * Struct to carry around connection (client)-specific data.
 */
static struct event_base *evbase_accept;
static workqueue_t workqueue;

/* Signal handler function (defined below). */
static void sighandler(int signal);

/**
 * Set a socket to non-blocking mode.
 */
static int set_non_block(int fd) {
	int flags;

	flags = fcntl(fd, F_GETFL);
	if (flags < 0) return flags;
	flags |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) < 0) return -1;
	return 0;
}

/**
 * This function will be called by libevent when there is a connection
 * ready to be accepted.
 */
void on_accept(int fd, short ev, void *arg) {
	workqueue_t *workqueue = (workqueue_t *)arg;
	job_t *job;
    /* Create a job object and add it to the work queue. */
	if ((job = malloc(sizeof(*job))) == NULL) {
		LOG_PRINT(LOG_WARNING, "Failed to allocate memory for job state");
		return;
	}
	job->user_data = fd;

	workqueue_add_job(workqueue, job);
}

/**
 * Run the server.  This function blocks, only returning when the server has terminated.
 */
int runServer(int port) {
	int listenfd;
	struct sockaddr_in listen_addr;
	struct event ev_accept;
	int reuseaddr_on;

	/* Initialize libevent. */
	event_init();

	/* Set signal handlers */
	sigset_t sigset;
	sigemptyset(&sigset);
	struct sigaction siginfo = {
		.sa_handler = sighandler,
		.sa_mask = sigset,
		.sa_flags = SA_RESTART,
	};
	sigaction(SIGINT, &siginfo, NULL);
	sigaction(SIGTERM, &siginfo, NULL);

	/* Create our listening socket. */
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) {
        LOG_PRINT(LOG_ERROR, "Setup Socket Failed!");
        return -1;
	}
	memset(&listen_addr, 0, sizeof(listen_addr));
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_addr.s_addr = INADDR_ANY;
	listen_addr.sin_port = htons(port);
	if (bind(listenfd, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0) {
        LOG_PRINT(LOG_ERROR, "Bind Socket Failed!");
        return -1;
	}
	if (listen(listenfd, CONNECTION_BACKLOG) < 0) {
        LOG_PRINT(LOG_ERROR, "Listen Socket Failed!");
        return -1;
	}
	reuseaddr_on = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr_on, sizeof(reuseaddr_on));

	/* Set the socket to non-blocking, this is essential in event
	 * based programming with libevent. */
	if (set_non_block(listenfd) < 0) {
		err(1, "failed to set server socket to non-blocking");
	}

	if ((evbase_accept = event_base_new()) == NULL) {
		LOG_PRINT(LOG_ERROR, "Unable to create socket accept event base");
		close(listenfd);
		return -1;
	}

	/* Initialize work queue. */
	if (workqueue_init(&workqueue, NUM_THREADS)) {
		LOG_PRINT(LOG_WARNING, "Failed to create work queue");
		close(listenfd);
		workqueue_shutdown(&workqueue);
		return -1;
	}

	/* We now have a listening socket, we create a read event to
	 * be notified when a client connects. */
	event_set(&ev_accept, listenfd, EV_READ|EV_PERSIST, on_accept, (void *)&workqueue);
	event_base_set(evbase_accept, &ev_accept);
	event_add(&ev_accept, NULL);

	LOG_PRINT(LOG_INFO, "Server running.");

	/* Start the event loop. */
	event_base_dispatch(evbase_accept);

	event_base_free(evbase_accept);
	evbase_accept = NULL;

	close(listenfd);

	LOG_PRINT(LOG_INFO, "Server shutdown.");

	return 0;
}

/**
 * Kill the server.  This function can be called from another thread to kill the
 * server, causing runServer() to return.
 */
void kill_server(void) {
	LOG_PRINT(LOG_INFO, "Stopping socket listener event loop.");
	if (event_base_loopexit(evbase_accept, NULL)) {
		LOG_PRINT(LOG_ERROR, "Error shutting down server");
	}
	LOG_PRINT(LOG_INFO, "Stopping workers.\n");
	workqueue_shutdown(&workqueue);
}

static void sighandler(int signal) {
	LOG_PRINT(LOG_INFO, "Received signal %d: %s.  Shutting down.", signal, strsignal(signal));
	kill_server();
}

