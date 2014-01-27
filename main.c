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
 * @file main.c
 * @brief Entrance of zimg.
 * @author 招牌疯子 zp@buaa.us
 * @version 1.0
 * @date 2013-07-19
 */

#include <evhtp.h>
#include <wand/MagickWand.h>
#include <inttypes.h>
#include <unistd.h>
#include <signal.h>
#include "zcommon.h"
#include "zhttpd.h"
#include "zutil.h"
#include "zlog.h"
#include "zcache.h"

extern struct setting settings;
evbase_t *evbase;

static void settings_init(void); 
static void sighandler(int signal); 
int main(int argc, char **argv);
void kill_server(void);


/**
 * @brief settings_init Init the setting with default values.
 */
static void settings_init(void) 
{
    settings.daemon = 0;
    strcpy(settings.root_path, "./www/index.html");
    strcpy(settings.img_path, "./img");
    strcpy(settings.log_name, "./log/zimg.log");
    settings.port = 4869;
    settings.backlog = 1024;
    //int th_n = get_cpu_cores();
    //printf("CPU cores: %d\n", th_n); 
    settings.num_threads = get_cpu_cores();         /* N workers */
    settings.log = false;
    settings.cache_on = false;
    strcpy(settings.cache_ip, "127.0.0.1");
    settings.cache_port = 11211;
    settings.max_keepalives = 1;
}

/**
 * @brief sighandler Signal process function.
 *
 * @param signal System signals.
 */
static void sighandler(int signal) 
{
    LOG_PRINT(LOG_INFO, "Received signal %d: %s.  Shutting down.", signal, strsignal(signal));
    kill_server();
}

/**
 * @brief main The entrance of zimg.
 *
 * @param argc Count of args.
 * @param argv Arg list.
 *
 * @return It returns a int to system.
 */
int main(int argc, char **argv)
{
    int c;
    _init_path = getcwd(NULL, 0);
    LOG_PRINT(LOG_INFO, "Get init-path: %s", _init_path);

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


    settings_init();
    while (-1 != (c = getopt(argc, argv,
                    "d"
                    "p:"
                    "t:"
                    "l"
                    "c"
                    "M:"
                    "m:"
                    "b:"
                    "h"
                    "k:"
                    )))
    {
        switch(c)
        {
            case 'd':
                settings.daemon = 1;
                break;
            case 'p':
                settings.port = atoi(optarg);
                break;
            case 't':
                settings.num_threads = atoi(optarg);
                if (settings.num_threads <= 0) {
                    fprintf(stderr, "Number of threads must be greater than 0\n");
                    return 1;
                }
                /* There're other problems when you get above 64 threads.
                 * In the future we should portably detect # of cores for the
                 * default.
                 */
                if (settings.num_threads > 64) {
                    fprintf(stderr, "WARNING: Setting a high number of worker"
                            "threads is not recommended.\n"
                            " Set this value to the number of cores in"
                            " your machine or less.\n");
                }
                break;
            case 'l':
                settings.log = true;
                break;
            case 'c':
                settings.cache_on = true;
                break;
            case 'M':
                strcpy(settings.cache_ip, optarg);
                break;
            case 'm':
                settings.cache_port = atoi(optarg);
                break;
            case 'b':
                settings.backlog = atoi(optarg);
                break;
            case 'k':
                settings.max_keepalives = atoll(optarg);
                break;
            case 'h':
                printf("Usage: ./zimg -d[aemon] -p port -t thread_num -M memcached_ip -m memcached_port -l[og] -c[ache] -b backlog_num -k max_keepalives -h[elp]\n");
                exit(1);
            default:
                fprintf(stderr, "Illegal argument \"%c\"\n", c);
                return 1;
        }
    }

    if(settings.daemon == 1)
    {
        if(daemon(1, 1) < 0)
        {
            fprintf(stderr, "Create daemon failed!\n");
        }
    }
    //init the Path zimg need to use.
    LOG_PRINT(LOG_INFO, "Begin to Init the Path zimg Using...");
//    if(is_dir(settings.root_path) != 1)
//    {
//        if(mk_dir(settings.root_path) != 1)
//        {
//            LOG_PRINT(LOG_ERROR, "root_path[%s] Create Failed!", settings.root_path);
//            return -1;
//        }
//    }
//
//
    //start log module... ./log/zimg.log
    if(settings.log)
    {
        const char *log_path = "./log";
        if(is_dir(log_path) != 1)
        {
            if(mk_dir(log_path) != 1)
            {
                LOG_PRINT(LOG_ERROR, "log_path[%s] Create Failed!", log_path);
                return -1;
            }
        }
        log_init();
    }

    if(is_dir(settings.img_path) != 1)
    {
        if(mk_dir(settings.img_path) != 1)
        {
            LOG_PRINT(LOG_ERROR, "img_path[%s] Create Failed!", settings.img_path);
            return -1;
        }
    }
    LOG_PRINT(LOG_INFO,"Paths Init Finished.");

   
    //init memcached connection...
    if(settings.cache_on == true)
    {
        LOG_PRINT(LOG_INFO, "Begin to Init Memcached Connection...");
        memcached_st *memc;
        memc= memcached_create(NULL);

        char mserver[32];
        sprintf(mserver, "%s:%d", settings.cache_ip, settings.cache_port);
        memcached_server_st *servers = memcached_servers_parse(mserver);

        memcached_server_push(memc, servers);
        memcached_server_list_free(servers);
        memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 0);
        //使用NO-BLOCK，防止memcache倒掉时挂死          
        memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NO_BLOCK, 1); 
        LOG_PRINT(LOG_INFO, "Memcached Connection Init Finished.");
        if(set_cache("zimg", "1") == -1)
        {
            LOG_PRINT(LOG_WARNING, "Memcached[%s] Connect Failed!", mserver);
            settings.cache_on = false;
        }
        else
        {
            LOG_PRINT(LOG_INFO, "memcached connection to: %s", mserver);
            settings.cache_on = true;
        }
        memcached_free(memc);
    }
    else
        LOG_PRINT(LOG_INFO, "Don't use memcached as cache.");
    //init magickwand
    MagickWandGenesis();

    //begin to start httpd...
    LOG_PRINT(LOG_INFO, "Begin to Start Httpd Server...");
    evbase = event_base_new();
    evhtp_t  * htp    = evhtp_new(evbase, NULL);

    evhtp_set_cb(htp, "/dump", dump_request_cb, NULL);
    evhtp_set_cb(htp, "/upload", post_request_cb, NULL);
    //evhtp_set_gencb(htp, echo_cb, NULL);
    evhtp_set_gencb(htp, send_document_cb, NULL);
#ifndef EVHTP_DISABLE_EVTHR
    evhtp_use_threads(htp, NULL, settings.num_threads, NULL);
#endif
    evhtp_set_max_keepalive_requests(htp, settings.max_keepalives);
    evhtp_bind_socket(htp, "0.0.0.0", settings.port, settings.backlog);

    event_base_loop(evbase, 0);

    evhtp_unbind_socket(htp);
    evhtp_free(htp);
    event_base_free(evbase);
    MagickWandTerminus();

    fprintf(stdout, "\nByebye!\n");
    return 0;
}


/**
 * @brief kill_server Kill threads and exit the event_base_loop.
 */
void kill_server(void)
{
    LOG_PRINT(LOG_INFO, "Stopping socket listener event loop.");
    if (event_base_loopexit(evbase, NULL)) {
        LOG_PRINT(LOG_ERROR, "Error shutting down server");
    }
    LOG_PRINT(LOG_INFO, "Stopping workers.\n");
}
