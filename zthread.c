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
#include "zhttpd.h"

/* Connection backlog (# of backlogged connections to accept). */
#define CONNECTION_BACKLOG 128
/* Number of worker threads.  Should match number of CPU cores reported in /proc/cpuinfo. */
#define NUM_THREADS 4

/**
 * Struct to carry around connection (client)-specific data.
 */
typedef struct client {
	/* The client's socket. */
	int fd;

	/* The event_base for this client. */
	struct event_base *evbase;

	/* The evhttp for this client. */
	struct evhttp *http;

	/* Here you can add your own application-specific attributes which
	 * are connection-specific. */
} client_t;

static struct event_base *evbase_accept;
static workqueue_t workqueue;

/* Signal handler function (defined below). */
static void sighandler(int signal);

/**
 * Set a socket to non-blocking mode.
 */
static int setnonblock(int fd) {
	int flags;

	flags = fcntl(fd, F_GETFL);
	if (flags < 0) return flags;
	flags |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) < 0) return -1;
	return 0;
}

static void closeClient(client_t *client) {
	if (client != NULL) {
		if (client->fd >= 0) {
			close(client->fd);
			client->fd = -1;
		}
	}
}

static void closeAndFreeClient(client_t *client) {
	if (client != NULL) {
		closeClient(client);
		if (client->http != NULL) {
			evhttp_free(client->http);
			client->http = NULL;
		}
		if (client->evbase != NULL) {
			event_base_free(client->evbase);
			client->evbase = NULL;
		}
		free(client);
	}
}


static void server_job_function(struct job *job) {
	client_t *client = (client_t *)job->user_data;

	event_base_dispatch(client->evbase);
	closeAndFreeClient(client);
	free(job);
}

/**
 * This function will be called by libevent when there is a connection
 * ready to be accepted.
 */
void on_accept(int fd, short ev, void *arg) {
	struct sockaddr_in client_addr;
	socklen_t client_len = sizeof(client_addr);
	workqueue_t *workqueue = (workqueue_t *)arg;
	client_t *client;
	job_t *job;
	/* Create a client object. */
	if ((client = malloc(sizeof(*client))) == NULL) {
		LOG_PRINT(LOG_WARNING, "Failed to allocate memory for client state");
		return;
	}
	memset(client, 0, sizeof(*client));

	/* Add any custom code anywhere from here to the end of this function
	 * to initialize your application-specific attributes in the client struct. */
	if ((client->evbase = event_base_new()) == NULL) {
		LOG_PRINT(LOG_WARNING, "Client event_base creation failed");
		closeAndFreeClient(client);
		return;
	}

	if ((client->http = evhttp_new(client->evbase)) == NULL) {
		LOG_PRINT(LOG_WARNING, "Client evhttp creation failed");
		closeAndFreeClient(client);
		return;
	}

    evhttp_set_cb(client->http, "/dump", dump_request_cb, NULL);
    evhttp_set_cb(client->http, "/upload", post_request_cb, NULL);
    evhttp_set_gencb(client->http, send_document_cb, NULL);
    struct evhttp_bound_socket *handle;
    handle = evhttp_accept_socket_with_handle(client->http, fd);
    if (handle != NULL) {
        LOG_PRINT(LOG_INFO, "Bound to port - Awaiting connections ... ");
    }
    else
        return;



	/* Create the buffered event.
	 *
	 * The first argument is the file descriptor that will trigger
	 * the events, in this case the clients socket.
	 *
	 * The second argument is the callback that will be called
	 * when data has been read from the socket and is available to
	 * the application.
	 *
	 * The third argument is a callback to a function that will be
	 * called when the write buffer has reached a low watermark.
	 * That usually means that when the write buffer is 0 length,
	 * this callback will be called.  It must be defined, but you
	 * don't actually have to do anything in this callback.
	 *
	 * The fourth argument is a callback that will be called when
	 * there is a socket error.  This is where you will detect
	 * that the client disconnected or other socket errors.
	 *
	 * The fifth and final argument is to store an argument in
	 * that will be passed to the callbacks.  We store the client
	 * object here.
	 */
	/* Create a job object and add it to the work queue. */
	if ((job = malloc(sizeof(*job))) == NULL) {
		LOG_PRINT(LOG_WARNING, "Failed to allocate memory for job state");
		closeAndFreeClient(client);
		return;
	}
	job->job_function = server_job_function;
	job->user_data = client;

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
	if (setnonblock(listenfd) < 0) {
		err(1, "failed to set server socket to non-blocking");
	}

	if ((evbase_accept = event_base_new()) == NULL) {
		LOG_PRINT(LOG_ERROR, "Unable to create socket accept event base");
		close(listenfd);
		return -1;
	}

	/* Initialize work queue. */
	if (workqueue_init(&workqueue, NUM_THREADS)) {
		LOG_PRINT(LOG_INFO, "Failed to create work queue");
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
void killServer(void) {
	LOG_PRINT(LOG_INFO, "Stopping socket listener event loop.");
	if (event_base_loopexit(evbase_accept, NULL)) {
		LOG_PRINT(LOG_ERROR, "Error shutting down server");
	}
	LOG_PRINT(LOG_INFO, "Stopping workers.\n");
	workqueue_shutdown(&workqueue);
}

static void sighandler(int signal) {
	LOG_PRINT(LOG_INFO, "Received signal %d: %s.  Shutting down.", signal, strsignal(signal));
	killServer();
}

