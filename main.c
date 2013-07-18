#include "zcommon.h"
#include "zlog.h"

extern struct setting settings;

static void settings_init(void) 
{
    strcpy(settings.root_path, "./www");
    strcpy(settings.img_path, "./img");
    settings.port = 4869;
    settings.backlog = 1024;
    settings.num_threads = 4;         /* N workers */
    settings.log = false;
    settings.cache_on = false;
    settings.cache_port = 11211;
}

static void sighandler(int signal) 
{
    LOG_PRINT(LOG_INFO, "Received signal %d: %s.  Shutting down.", signal, strsignal(signal));
    kill_server();
}

int main(int argc, char **argv)
{
    int c;
    char *log_path = "log/zimg.log";
    extern int _log_id;
    _log_id = -1;
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
                    "p:"
                    "t:"
                    "l"
                    "m:"
                    "d"
                    )))
    {
        switch(c)
        {
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
            case 'm':
                settings.cache_port = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Illegal argument \"%c\"\n", c);
                return 1;
        }
    }

    //start log module... ./log/zimg.log
    if(settings.log)
    {
        log_init();
        _log_id = log_open(log_path, "a");
    }

    //init the Path zimg need to use.
    LOG_PRINT(LOG_INFO, "Begin to Init the Path zimg Using...");
    if(is_dir(settings.root_path) != 1)
    {
        if(mk_dir(settings.root_path) != 1)
        {
            LOG_PRINT(LOG_ERROR, "root_path[%s] Create Failed!", settings.root_path);
            return -1;
        }
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
    LOG_PRINT(LOG_INFO, "Begin to Init Memcached Connection...");
    _memc= memcached_create(NULL);

    char mserver[32];
    sprintf(mserver, "127.0.0.1:%d", settings.cache_port);
    memcached_server_st *servers = memcached_servers_parse(mserver);

    memcached_server_push(_memc, servers);
    memcached_server_list_free(servers);
    memcached_behavior_set(_memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 0);
    //使用NO-BLOCK，防止memcache倒掉时挂死          
    memcached_behavior_set(_memc, MEMCACHED_BEHAVIOR_NO_BLOCK, 1); 
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

    //begin to start httpd...
    LOG_PRINT(LOG_INFO, "Begin to Start Httpd Server...");
    if(runServer(settings.port) == -1)
    {
        LOG_PRINT(LOG_ERROR, "zhttpd start failed.");
    }

    memcached_free(_memc);
    log_close(_log_id);
    fprintf(stdout, "\nByebye!\n");
    return 0;
}
