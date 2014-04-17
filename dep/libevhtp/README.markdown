# Libevhtp
*****

This document describes details on using the evhtp API. This document is
probably not very awesome, it's best to look at test.c to see advanced usage.

## Required Dependencies
* [gcc](http://gcc.gnu.org/)
* [Libevent2](http://libevent.org)

## Optional Dependencies
* [OpenSSL](http://openssl.org)
* pthreads

## Building
* cd build
* cmake ..
* make
* make examples

## Overview
***

Libevhtp was created as a replacement API for Libevent's current HTTP API.  The reality of libevent's http interface is that it was created as a JIT server, meaning the developer never thought of it being used for creating a full-fledged HTTP service. Infact I am under the impression that the libevent http API was designed almost as an example of what you can do with libevent. It's not Apache in a box, but more and more developers are attempting to use it as so.

### Libevent's HTTP pitfalls
***

* It was not designed to be a fully functional HTTP server.
* The code is messy, abstractions are almost non-existent, and feature-creep has made long-term maintainability very hard.
* The parsing code is slow and requires data to be buffered before a full parse can be completed. This results in extranious memory usage and lots of string comparison functions.
* There is no method for a user to access various parts of the request processing cycle. For example if the "Content-Length" header has a value of 50000, your callback is not executed until all 50000 bytes have been read.
* Setting callback URI's do exact matches; meaning if you set a callback for "/foo/", requests for "/foo/bar/" are ignored.
* Creating an HTTPS server is hard, it requires a bunch of work to be done on the underlying bufferevents.
* As far as I know, streaming data back to a client is hard, if not impossible without messing with underlying bufferevents.
* It's confusing to work with, this is probably due to the lack of proper documentation.

Libevhtp attempts to address these problems along with a wide variety of cool mechanisms allowing a developer to have complete control over your server operations. This is not to say the API cannot be used in a very simplistic manner - a developer can easily create a backwards compatible version of libevent's HTTP server to libevhtp.

### A bit about the architecture of libevhtp
***

#### Bootstrapping

1.	Create a parent evhtp_t structure.
2.	Assign callbacks to the parent for specific URIs or posix-regex based URI's
3.	Optionally assign per-connection hooks (see hooks) to the callbacks.
4.	Optionally assign pre-accept and post-accept callbacks for incoming connections.	
5.	Optionally enable built-in threadpool for connection handling (lock-free, and non-blocking).
6.	Optionally morph your server to HTTPS.
7.	Start the evhtp listener.

#### Request handling.

1.	Optionally deal with pre-accept and post-accept callbacks if they exist, allowing for a connection to be rejected if the function deems it as unacceptable.
2.	Optionally assign per-request hooks (see hooks) for a request (the most optimal place for setting these hooks is on a post-accept callback).
3.	Deal with either per-connection or per-request hook callbacks if they exist.
4.	Once the request has been fully processed, inform evhtp to send a reply.

##### A very basic example with no optional conditions.

	#include <stdio.h>
	#include <evhtp.h>

	void
	testcb(evhtp_request_t * req, void * a) {
	    evbuffer_add_reference(req->buffer_out, "foobar", 6, NULL, NULL);
	    evhtp_send_reply(req, EVHTP_RES_OK);
	}

	int
	main(int argc, char ** argv) {
	    evbase_t * evbase = event_base_new();
	    evhtp_t  * htp    = evhtp_new(evbase, NULL);
	
	    evhtp_set_cb(htp, "/test", testcb, NULL);
	    evhtp_bind_socket(htp, "0.0.0.0", 8080, 1024);
	    event_base_loop(evbase, 0);
	    return 0;
	}


## Is evhtp thread-safe?

For simple usage with evhtp_use_threads(), yes. But for more extreme cases:
sorta, you are bound to the thread mechanisms of libevent itself. 

But with proper design around libevhtp, thread issues can be out-of-sight,
out-of-mind. 

What do you mean by this "proper design" statement? 

Refer to the code in ./examples/thread_design.c. The comments go into great detail
of the hows and whys for proper design using libevhtp's threading model.

This example uses redis, mainly because most people who have asked me "is evhtp
thread-safe" were attempting to *other things* before sending a response to a
request. And on more than one occasion, those *other things* were communicating
with redis.


## For Windows MinGW

  	cmake -G "MSYS Makefiles" -DCMAKE_INCLUDE_PATH=/mingw/include -DCMAKE_LIBRARY_PATH=/mingw/lib -DCMAKE_INSTALL_PREFIX=/mingw  .

	make
