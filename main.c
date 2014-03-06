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
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

extern struct setting settings;
evbase_t *evbase;

void usage(int argc, char **argv);
static void settings_init(void); 
static int load_conf(const char *conf); 
static void sighandler(int signal); 
int main(int argc, char **argv);
void kill_server(void);


void usage(int argc, char **argv)
{
    printf("Usage:\n");
    printf("    %s [-d] /path/to/zimg.lua\n", argv[0]);
    printf("Options:\n");
    printf("    -d    run as daemon\n");
}

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
    settings.num_threads = get_cpu_cores();         /* N workers */
    settings.log = false;
    settings.cache_on = false;
    strcpy(settings.cache_ip, "127.0.0.1");
    settings.cache_port = 11211;
    settings.max_keepalives = 1;
}

static int load_conf(const char *conf)
{
    int result = -1;
    lua_State *L = lua_open();
    luaL_openlibs(L);
    if (luaL_loadfile(L, conf) || lua_pcall(L, 0, 0, 0))
    {
        lua_close(L);
        return -1;
    }

    lua_getglobal(L, "daemon"); //stack index: -12
    lua_getglobal(L, "port");
    lua_getglobal(L, "thread_num");
    lua_getglobal(L, "cache");
    lua_getglobal(L, "mc_ip");
    lua_getglobal(L, "mc_port");
    lua_getglobal(L, "log");
    lua_getglobal(L, "backlog_num");
    lua_getglobal(L, "max_keepalives");

    lua_getglobal(L, "root_path");
    lua_getglobal(L, "img_path");
    lua_getglobal(L, "log_name"); //stack index: -1

    if(lua_isnumber(L, -12))
        settings.daemon = (int)lua_tonumber(L, -12);
    if(lua_isnumber(L, -11))
        settings.port = (int)lua_tonumber(L, -11);
    printf("settings.port: %d\n", settings.port);
    if(lua_isnumber(L, -10))
        settings.num_threads = (int)lua_tonumber(L, -10);         /* N workers */
    printf("settings.num_threads: %d\n", settings.num_threads);
    if(lua_isnumber(L, -9))
        settings.cache_on = (int)lua_tonumber(L, -9);
    printf("settings.cache_on: %d\n", settings.cache_on);
    if(lua_isstring(L, -8))
        strcpy(settings.cache_ip, lua_tostring(L, -8));
    printf("settings.cache_ip: %s\n", settings.cache_ip);
    if(lua_isnumber(L, -7))
        settings.cache_port = (int)lua_tonumber(L, -7);
    if(lua_isnumber(L, -6))
        settings.log = (int)lua_tonumber(L, -6);
    if(lua_isnumber(L, -5))
        settings.backlog = (int)lua_tonumber(L, -5);
    if(lua_isnumber(L, -4))
        settings.max_keepalives = (int)lua_tonumber(L, -4);

    if(lua_isstring(L, -3))
        strcpy(settings.root_path, lua_tostring(L, -3));
    printf("settings.root_path: %s\n", settings.root_path);
    if(lua_isstring(L, -2))
        strcpy(settings.img_path, lua_tostring(L, -2));
    if(lua_isstring(L, -1))
        strcpy(settings.log_name, lua_tostring(L, -1));

    lua_close(L);
    return 1;
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

    if(argc < 2)
    {
        usage(argc, argv);
        return -1;
    }

    settings_init();
    const char *conf_file = NULL;
    for(int i=1; i<argc; i++)
    {
        if(strcmp(argv[i], "-d") == 0){
            settings.daemon = 1;
        }else{
            conf_file = argv[i];
        }
    }

    if(conf_file == NULL)
    {
        usage(argc, argv);
        return -1;
    }

    if(is_file(conf_file) == -1)
    {
        fprintf(stderr, "'%s' is not a file or not exists!\n", conf_file);
        return -1;
    }

    if(load_conf(conf_file) == -1)
    {
        fprintf(stderr, "'%s' load failed!\n", conf_file);
        return -1;
    }

    if(settings.daemon == 1)
    {
        if(daemon(1, 1) < 0)
        {
            fprintf(stderr, "Create daemon failed!\n");
            return -1;
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
