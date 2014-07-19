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

#if __APPLE__
// In Mac OS X 10.5 and later trying to use the daemon function gives a “‘daemon’ is deprecated”
// error, which prevents compilation because we build with "-Werror".
// Since this is supposed to be portable cross-platform code, we don't care that daemon is
// deprecated on Mac OS X 10.5, so we use this preprocessor trick to eliminate the error message.
#define daemon yes_we_know_that_daemon_is_deprecated_in_os_x_10_5_thankyou
#endif

#include <stdio.h>
#include <fcntl.h>
#include <wand/MagickWand.h>
#include <inttypes.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"
#include <evhtp-config.h>
#include "libevhtp/evhtp.h"
#include "zcommon.h"
#include "zhttpd.h"
#include "zutil.h"
#include "zlog.h"
#include "zcache.h"

#if __APPLE__
#undef daemon
extern int daemon(int, int);
#endif

#define _STR(s) #s   
#define STR(s) _STR(s)

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
    settings.is_daemon = 0;
    settings.port = 4869;
    settings.num_threads = get_cpu_cores();         /* N workers */
    settings.backlog = 1024;
    settings.max_keepalives = 1;
    settings.retry = 3;
    str_lcpy(settings.version, STR(ZIMG_VERSION), sizeof(settings.version));
    snprintf(settings.server_name, 128, "zimg/%s", settings.version);
    settings.headers = NULL;
    settings.etag = 0;
    settings.up_access = NULL;
    settings.down_access = NULL;
    settings.cache_on = 0;
    str_lcpy(settings.cache_ip, "127.0.0.1", sizeof(settings.cache_ip));
    settings.cache_port = 11211;
    settings.log = 0;
    str_lcpy(settings.log_name, "./log/zimg.log", sizeof(settings.log_name));
    str_lcpy(settings.root_path, "./www/index.html", sizeof(settings.root_path));
    str_lcpy(settings.dst_format, "JPEG", sizeof(settings.dst_format));
    settings.mode = 1;
    settings.save_new = 1;
    str_lcpy(settings.img_path, "./img", sizeof(settings.img_path));
    str_lcpy(settings.beansdb_ip, "127.0.0.1", sizeof(settings.beansdb_ip));
    settings.beansdb_port = 7905;
    str_lcpy(settings.ssdb_ip, "127.0.0.1", sizeof(settings.ssdb_ip));
    settings.ssdb_port = 6379;
}

static int load_conf(const char *conf)
{
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    if (luaL_loadfile(L, conf) || lua_pcall(L, 0, 0, 0))
    {
        lua_close(L);
        return -1;
    }

    lua_getglobal(L, "is_daemon"); //stack index: -12
    if(lua_isnumber(L, -1))
        settings.is_daemon = (int)lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getglobal(L, "port");
    if(lua_isnumber(L, -1))
        settings.port = (int)lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getglobal(L, "thread_num");
    if(lua_isnumber(L, -1))
        settings.num_threads = (int)lua_tonumber(L, -1);         /* N workers */
    lua_pop(L, 1);

    lua_getglobal(L, "backlog_num");
    if(lua_isnumber(L, -1))
        settings.backlog = (int)lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getglobal(L, "max_keepalives");
    if(lua_isnumber(L, -1))
        settings.max_keepalives = (int)lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getglobal(L, "retry");
    if(lua_isnumber(L, -1))
        settings.retry = (int)lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getglobal(L, "system");
    if(lua_isstring(L, -1))
    {
        char tmp[128];
        snprintf(tmp, 128, "%s %s", settings.server_name, lua_tostring(L, -1));
        snprintf(settings.server_name, 128, "%s", tmp);
    }
    lua_pop(L, 1);

    lua_getglobal(L, "headers");
    if(lua_isstring(L, -1))
    {
        settings.headers = conf_get_headers(lua_tostring(L, -1));
    }
    lua_pop(L, 1);

    lua_getglobal(L, "etag");
    if(lua_isnumber(L, -1))
        settings.etag = (int)lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getglobal(L, "upload_rule");
    if(lua_isstring(L, -1))
    {
        settings.up_access = conf_get_rules(lua_tostring(L, -1));
    }
    lua_pop(L, 1);

    lua_getglobal(L, "download_rule");
    if(lua_isstring(L, -1))
    {
        settings.down_access = conf_get_rules(lua_tostring(L, -1));
    }
    lua_pop(L, 1);

    lua_getglobal(L, "cache");
    if(lua_isnumber(L, -1))
        settings.cache_on = (int)lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getglobal(L, "mc_ip");
    if(lua_isstring(L, -1))
        str_lcpy(settings.cache_ip, lua_tostring(L, -1), sizeof(settings.cache_ip));
    lua_pop(L, 1);

    lua_getglobal(L, "mc_port");
    if(lua_isnumber(L, -1))
        settings.cache_port = (int)lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getglobal(L, "log");
    if(lua_isnumber(L, -1))
        settings.log = (int)lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getglobal(L, "log_name"); //stack index: -1
    if(lua_isstring(L, -1))
        str_lcpy(settings.log_name, lua_tostring(L, -1), sizeof(settings.log_name));
    lua_pop(L, 1);

    lua_getglobal(L, "root_path");
    if(lua_isstring(L, -1))
        str_lcpy(settings.root_path, lua_tostring(L, -1), sizeof(settings.root_path));
    lua_pop(L, 1);

    int format = 1;
    lua_getglobal(L, "format");
    if(lua_isnumber(L, -1))
        format = (int)lua_tonumber(L, -1);
    lua_pop(L, 1);
    //LOG_PRINT(LOG_INFO, "format = %d", format);
    if (format == 2) {
        str_lcpy(settings.dst_format, "WEBP", sizeof(settings.dst_format));
    } else if (format == 0) {
        settings.dst_format[0] = '\0';
    }
    //if (settings.dst_format[0] != '\0')
    //    LOG_PRINT(LOG_INFO, "settings.format = %s", settings.dst_format);
    //else
    //    LOG_PRINT(LOG_INFO, "settings.format = nil");

    lua_getglobal(L, "mode");
    if(lua_isnumber(L, -1))
        settings.mode = (int)lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getglobal(L, "save_new");
    if(lua_isnumber(L, -1))
        settings.save_new = (int)lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getglobal(L, "img_path");
    if(lua_isstring(L, -1))
        str_lcpy(settings.img_path, lua_tostring(L, -1), sizeof(settings.img_path));
    lua_pop(L, 1);

    lua_getglobal(L, "beansdb_ip");
    if(lua_isstring(L, -1))
        str_lcpy(settings.beansdb_ip, lua_tostring(L, -1), sizeof(settings.beansdb_ip));
    lua_pop(L, 1);

    lua_getglobal(L, "beansdb_port");
    if(lua_isnumber(L, -1))
        settings.beansdb_port= (int)lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getglobal(L, "ssdb_ip");
    if(lua_isstring(L, -1))
        str_lcpy(settings.ssdb_ip, lua_tostring(L, -1), sizeof(settings.ssdb_ip));
    lua_pop(L, 1);

    lua_getglobal(L, "ssdb_port");
    if(lua_isnumber(L, -1))
        settings.ssdb_port = (int)lua_tonumber(L, -1);
    lua_pop(L, 1);
    
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
    char msg[128];
    msg[0] = '\0';
    str_lcat(msg, "----/--/-- --:--:--:------ [INFO] signal Terminal received zimg shutting down", sizeof(msg));
    //str_lcat(msg, strsignal(signal));
    log_handler(msg);
    write(STDOUT_FILENO, "\nbye bye!\n", 10);

    _exit(1);
}

void init_thread(evhtp_t *htp, evthr_t *thread, void *arg)
{
    thr_arg_t *thr_args;
    thr_args = calloc(1, sizeof(thr_arg_t));
    thr_args->thread = thread;

    char mserver[32];

    if(settings.cache_on == true)
    {
        memcached_st *memc = memcached_create(NULL);
        snprintf(mserver, 32, "%s:%d", settings.cache_ip, settings.cache_port);
        memcached_server_st *servers = memcached_servers_parse(mserver);
        memcached_server_push(memc, servers);
        memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 1);
        memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NO_BLOCK, 1); 
        memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NOREPLY, 1); 
        memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_TCP_KEEPALIVE, 1); 
        thr_args->cache_conn = memc;
        LOG_PRINT(LOG_DEBUG, "Memcached Connection Init Finished.");
        memcached_server_list_free(servers);
    }

    if(settings.mode == 2)
    {
        memcached_st *beans = memcached_create(NULL);
        snprintf(mserver, 32, "%s:%d", settings.beansdb_ip, settings.beansdb_port);
        memcached_server_st *servers = memcached_servers_parse(mserver);
        servers = memcached_servers_parse(mserver);
        memcached_server_push(beans, servers);
        memcached_behavior_set(beans, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 0);
        memcached_behavior_set(beans, MEMCACHED_BEHAVIOR_NO_BLOCK, 1); 
        memcached_behavior_set(beans, MEMCACHED_BEHAVIOR_TCP_KEEPALIVE, 1); 
        thr_args->beansdb_conn = beans;
        LOG_PRINT(LOG_DEBUG, "beansdb Connection Init Finished.");
        memcached_server_list_free(servers);
    }
    else if(settings.mode == 3)
    {
        redisContext* c = redisConnect(settings.ssdb_ip, settings.ssdb_port);
        if(c->err)
        {
            redisFree(c);
            LOG_PRINT(LOG_DEBUG, "Connect to ssdb server faile");
        }
        else
        {
            thr_args->ssdb_conn = c;
            LOG_PRINT(LOG_DEBUG, "Connect to ssdb server Success");
        }
    }

    evthr_set_aux(thread, thr_args);
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
    int i;
    retry_sleep.tv_sec = 0;
    retry_sleep.tv_nsec = RETRY_TIME_WAIT;      //1000 ns = 1 us

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
    for(i=1; i<argc; i++)
    {
        if(strcmp(argv[i], "-d") == 0){
            settings.is_daemon = 1;
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

    if(bind_check(settings.port) == -1)
    {
        fprintf(stderr, "Port %d bind failed!\n", settings.port);
        return -1;
    }

    if(settings.is_daemon == 1)
    {
        if(daemon(1, 1) < 0)
        {
            fprintf(stderr, "Create daemon failed!\n");
            return -1;
        }
        else
        {
            fprintf(stdout, "zimg %s\n", settings.version);
            fprintf(stdout, "Copyright (c) 2013-2014 zimg.buaa.us\n");
            fprintf(stderr, "\n");
        }
    }
    //init the Path zimg need to use.
    //start log module... ./log/zimg.log
    if(settings.log)
    {
        if(mk_dirf(settings.log_name) != 1)
        {
            fprintf(stderr, "%s log path create failed!\n", settings.log_name);
            return -1;
        }
        log_init();
    }

    if(is_dir(settings.img_path) != 1)
    {
        if(mk_dirs(settings.img_path) != 1)
        {
            LOG_PRINT(LOG_DEBUG, "img_path[%s] Create Failed!", settings.img_path);
            fprintf(stderr, "%s Create Failed!\n", settings.img_path);
            return -1;
        }
    }
    LOG_PRINT(LOG_DEBUG,"Paths Init Finished.");

    if(settings.mode == 2)
    {
        LOG_PRINT(LOG_DEBUG, "Begin to Test Memcached Connection...");
        memcached_st *beans = memcached_create(NULL);
        char mserver[32];
        snprintf(mserver, 32, "%s:%d", settings.beansdb_ip, settings.beansdb_port);
        memcached_server_st *servers = memcached_servers_parse(mserver);
        servers = memcached_servers_parse(mserver);
        memcached_server_push(beans, servers);
        memcached_server_list_free(servers);
        memcached_behavior_set(beans, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 0);
        memcached_behavior_set(beans, MEMCACHED_BEHAVIOR_NO_BLOCK, 1); 
        memcached_behavior_set(beans, MEMCACHED_BEHAVIOR_TCP_KEEPALIVE, 1); 
        LOG_PRINT(LOG_DEBUG, "beansdb Connection Init Finished.");
        if(set_cache(beans, "zimg", "1") == -1)
        {
            LOG_PRINT(LOG_DEBUG, "Beansdb[%s] Connect Failed!", mserver);
            fprintf(stderr, "Beansdb[%s] Connect Failed!\n", mserver);
            memcached_free(beans);
            return -1;
        }
        else
        {
            LOG_PRINT(LOG_DEBUG, "beansdb connected to: %s", mserver);
        }
        memcached_free(beans);
    }
    else if(settings.mode == 3)
    {
        redisContext* c = redisConnect(settings.ssdb_ip, settings.ssdb_port);
        if(c->err)
        {
            redisFree(c);
            LOG_PRINT(LOG_DEBUG, "Connect to ssdb server faile");
            fprintf(stderr, "SSDB[%s:%d] Connect Failed!\n", settings.ssdb_ip, settings.ssdb_port);
            return -1;
        }
        else
        {
            LOG_PRINT(LOG_DEBUG, "Connect to ssdb server Success");
        }
    }

    //init magickwand
    MagickWandGenesis();

    //begin to start httpd...
    LOG_PRINT(LOG_DEBUG, "Begin to Start Httpd Server...");
    LOG_PRINT(LOG_INFO, "zimg started");
    evbase = event_base_new();
    evhtp_t *htp = evhtp_new(evbase, NULL);

    evhtp_set_cb(htp, "/dump", dump_request_cb, NULL);
    evhtp_set_cb(htp, "/upload", post_request_cb, NULL);
    //evhtp_set_gencb(htp, echo_cb, NULL);
    evhtp_set_gencb(htp, send_document_cb, NULL);
#ifndef EVHTP_DISABLE_EVTHR
    evhtp_use_threads(htp, init_thread, settings.num_threads, NULL);
#endif
    evhtp_set_max_keepalive_requests(htp, settings.max_keepalives);
    evhtp_bind_socket(htp, "0.0.0.0", settings.port, settings.backlog);

    event_base_loop(evbase, 0);

    evhtp_unbind_socket(htp);
    evhtp_free(htp);
    event_base_free(evbase);
    MagickWandTerminus();
    free_headers_conf(settings.headers);
    free_access_conf(settings.up_access);
    free_access_conf(settings.down_access);

    //fprintf(stdout, "\nByebye!\n");
    return 0;
}


/**
 * @brief kill_server Kill threads and exit the event_base_loop.
 */
void kill_server(void)
{
    if (event_base_loopexit(evbase, NULL)) {
        LOG_PRINT(LOG_DEBUG, "Error shutting down server");
    }
    LOG_PRINT(LOG_INFO, "zimg stop");
}
