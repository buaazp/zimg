/*   
 *   zimg - high performance image storage and processing system.
 *       http://zimg.buaa.us 
 *   
 *   Copyright (c) 2013-2014, Peter Zhao <zp@buaa.us>.
 *   All rights reserved.
 *   
 *   Use and distribution licensed under the BSD license.
 *   See the LICENSE file for full text.
 * 
 */

/**
 * @file main.c
 * @brief the entrance of zimg
 * @author 招牌疯子 zp@buaa.us
 * @version 3.0.0
 * @date 2014-08-14
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
#include <inttypes.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include "libevhtp/evhtp.h"
#include "zcommon.h"
#include "zhttpd.h"
#include "zimg.h"
#include "zdb.h"
#include "zutil.h"
#include "zlog.h"
#include "zcache.h"
#include "zlscale.h"

#if __APPLE__
#undef daemon
extern int daemon(int, int);
#endif

#define _STR(s) #s   
#define STR(s) _STR(s)


void usage(int argc, char **argv);
static void settings_init(void); 
static int load_conf(const char *conf); 
static void sighandler(int signal, siginfo_t *siginfo, void *arg);
void init_thread(evhtp_t *htp, evthr_t *thread, void *arg);
int main(int argc, char **argv);

evbase_t *evbase;

extern const struct luaL_reg zimg_lib[];
extern const struct luaL_Reg loglib[];

/**
 * @brief usage usage display of zimg
 *
 * @param argc the arg count
 * @param argv the args
 */
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
    settings.L = NULL;
    settings.is_daemon = 0;
    str_lcpy(settings.ip, "0.0.0.0", sizeof(settings.ip));
    settings.port = 4869;
    settings.num_threads = get_cpu_cores();         /* N workers */
    settings.backlog = 1024;
    settings.max_keepalives = 1;
    settings.retry = 3;
    str_lcpy(settings.version, STR(PROJECT_VERSION), sizeof(settings.version));
    snprintf(settings.server_name, 128, "zimg/%s", settings.version);
    settings.headers = NULL;
    settings.etag = 0;
    settings.up_access = NULL;
    settings.down_access = NULL;
    settings.admin_access = NULL;
    settings.cache_on = 0;
    str_lcpy(settings.cache_ip, "127.0.0.1", sizeof(settings.cache_ip));
    settings.cache_port = 11211;
    settings.log_level = 6;
    str_lcpy(settings.log_name, "./log/zimg.log", sizeof(settings.log_name));
    str_lcpy(settings.root_path, "./www/index.html", sizeof(settings.root_path));
    str_lcpy(settings.admin_path, "./www/admin.html", sizeof(settings.admin_path));
    settings.disable_args = 0;
    settings.disable_type = 0;
    settings.script_on = 0;
    settings.script_name[0] = '\0';
    str_lcpy(settings.format, "none", sizeof(settings.format));
    settings.quality = 75;
    settings.mode = 1;
    settings.save_new = 1;
    settings.max_size = 10485760;
    str_lcpy(settings.img_path, "./img", sizeof(settings.img_path));
    str_lcpy(settings.beansdb_ip, "127.0.0.1", sizeof(settings.beansdb_ip));
    settings.beansdb_port = 7905;
    str_lcpy(settings.ssdb_ip, "127.0.0.1", sizeof(settings.ssdb_ip));
    settings.ssdb_port = 6379;
    multipart_parser_settings *callbacks = (multipart_parser_settings *)malloc(sizeof(multipart_parser_settings));
    memset(callbacks, 0, sizeof(multipart_parser_settings));
    //callbacks->on_header_field = on_header_field;
    callbacks->on_header_value = on_header_value;
    callbacks->on_chunk_data = on_chunk_data;
    settings.mp_set = callbacks;
    settings.get_img = NULL;
    settings.info_img = NULL;
    settings.admin_img = NULL;
}

static void set_callback(int mode)
{
    if(mode == 1)
    {
        settings.get_img = get_img;
        settings.info_img = info_img;
        settings.admin_img = admin_img;
    }
    else
    {
        settings.get_img = get_img_mode_db;
        settings.info_img = info_img_mode_db;
        settings.admin_img = admin_img_mode_db;
    }
}

/**
 * @brief load_conf load the conf of zimg
 *
 * @param conf conf name
 *
 * @return 1 for OK and -1 for fail
 */
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

    lua_getglobal(L, "ip");
    if(lua_isstring(L, -1))
        str_lcpy(settings.ip, lua_tostring(L, -1), sizeof(settings.ip));
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

    lua_getglobal(L, "admin_rule");
    if(lua_isstring(L, -1))
    {
        settings.admin_access = conf_get_rules(lua_tostring(L, -1));
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

    lua_getglobal(L, "log_level");
    if(lua_isnumber(L, -1))
        settings.log_level = (int)lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getglobal(L, "log_name"); //stack index: -1
    if(lua_isstring(L, -1))
        str_lcpy(settings.log_name, lua_tostring(L, -1), sizeof(settings.log_name));
    lua_pop(L, 1);

    lua_getglobal(L, "root_path");
    if(lua_isstring(L, -1))
        str_lcpy(settings.root_path, lua_tostring(L, -1), sizeof(settings.root_path));
    lua_pop(L, 1);

    lua_getglobal(L, "admin_path");
    if(lua_isstring(L, -1))
        str_lcpy(settings.admin_path, lua_tostring(L, -1), sizeof(settings.admin_path));
    lua_pop(L, 1);

    lua_getglobal(L, "disable_args");
    if(lua_isnumber(L, -1))
        settings.disable_args = (int)lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getglobal(L, "disable_type");
    if(lua_isnumber(L, -1))
        settings.disable_type = (int)lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getglobal(L, "script_name"); //stack index: -1
    if(lua_isstring(L, -1))
        str_lcpy(settings.script_name, lua_tostring(L, -1), sizeof(settings.script_name));
    lua_pop(L, 1);

    lua_getglobal(L, "format");
    if(lua_isstring(L, -1))
        str_lcpy(settings.format, lua_tostring(L, -1), sizeof(settings.format));
    lua_pop(L, 1);

    lua_getglobal(L, "quality");
    if(lua_isnumber(L, -1))
        settings.quality = (int)lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getglobal(L, "mode");
    if(lua_isnumber(L, -1))
        settings.mode = (int)lua_tonumber(L, -1);
    lua_pop(L, 1);

    set_callback(settings.mode);

    lua_getglobal(L, "save_new");
    if(lua_isnumber(L, -1))
        settings.save_new = (int)lua_tonumber(L, -1);
    lua_pop(L, 1);

    lua_getglobal(L, "max_size");
    if(lua_isnumber(L, -1))
        settings.max_size = (int)lua_tonumber(L, -1);
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

    settings.L = L;
    //lua_close(L);

    return 1;
}
/**
 * @brief sighandler the signal handler of zimg
 *
 * @param signal the signal zimg get
 * @param siginfo signal info
 * @param arg the arg for handler
 */
static void sighandler(int signal, siginfo_t *siginfo, void *arg)
{
    char msg[128];
    msg[0] = '\0';
    str_lcat(msg, "----/--/-- --:--:--:------ [INFO] signal Terminal received zimg shutting down", sizeof(msg));
    //str_lcat(msg, strsignal(signal));
    log_handler(msg);
    write(STDOUT_FILENO, "\nbye bye!\n", 10);

    //evbase_t *evbase = (evbase_t *)arg;
    struct timeval tv = { .tv_usec = 100000, .tv_sec = 0 }; /* 100 ms */
    event_base_loopexit(evbase, &tv);
}

/**
 * @brief init_thread the init function of threads
 *
 * @param htp evhtp object
 * @param thread the current thread
 * @param arg the arg for init
 */
void init_thread(evhtp_t *htp, evthr_t *thread, void *arg)
{
    thr_arg_t *thr_args;
    thr_args = calloc(1, sizeof(thr_arg_t));
    LOG_PRINT(LOG_DEBUG, "thr_args alloc");
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
    else
        thr_args->cache_conn = NULL;

    if(settings.mode == 2)
    {
        thr_args->ssdb_conn = NULL;
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
        thr_args->beansdb_conn = NULL;
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

    thr_args->L = luaL_newstate(); 
    LOG_PRINT(LOG_DEBUG, "luaL_newstate alloc");
    if(thr_args->L != NULL)
    {
        luaL_openlibs(thr_args->L);
        luaL_openlib(thr_args->L, "zimg", zimg_lib, 0);
        luaL_openlib(thr_args->L, "log", loglib, 0);
    }
    luaL_loadfile(thr_args->L, settings.script_name);
    lua_pcall(thr_args->L, 0, 0, 0);

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
    /* Set signal handlers */
    sigset_t sigset;
    sigemptyset(&sigset);
    struct sigaction siginfo = {
        .sa_sigaction = &sighandler,
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
    int i;
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
    if(mk_dirf(settings.log_name) != 1)
    {
        fprintf(stderr, "%s log path create failed!\n", settings.log_name);
        return -1;
    }
    log_init();

    if(settings.script_name[0] != '\0')
    {
        if(is_file(settings.script_name) == -1)
        {
            fprintf(stderr, "%s open failed!\n", settings.script_name);
            return -1;
        }
        settings.script_on = 1;
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
    MagickCoreGenesis((char *) NULL, MagickFalse);
    /*
    ExceptionInfo *exception=AcquireExceptionInfo();
    MagickInfo *jpeg_info = (MagickInfo *)GetMagickInfo("JPEG", exception);
    if(jpeg_info->thread_support != MagickTrue)
        LOG_PRINT(LOG_DEBUG, "thread_support != MagickTrue");
    jpeg_info->thread_support = MagickTrue;
    if(jpeg_info->thread_support != MagickTrue)
        LOG_PRINT(LOG_DEBUG, "thread_support != MagickTrue");
    MagickInfo *jpg_info = (MagickInfo *)GetMagickInfo("JPG", exception);
    jpg_info->thread_support = MagickTrue;
    */

    //begin to start httpd...
    LOG_PRINT(LOG_DEBUG, "Begin to Start Httpd Server...");
    LOG_PRINT(LOG_INFO, "zimg started");
    evbase = event_base_new();
    evhtp_t *htp = evhtp_new(evbase, NULL);

    evhtp_set_cb(htp, "/dump", dump_request_cb, NULL);
    evhtp_set_cb(htp, "/upload", post_request_cb, NULL);
    evhtp_set_cb(htp, "/admin", admin_request_cb, NULL);
    evhtp_set_cb(htp, "/info", info_request_cb, NULL);
    evhtp_set_cb(htp, "/echo", echo_cb, NULL);
    evhtp_set_gencb(htp, get_request_cb, NULL);
#ifndef EVHTP_DISABLE_EVTHR
    evhtp_use_threads(htp, init_thread, settings.num_threads, NULL);
#endif
    evhtp_set_max_keepalive_requests(htp, settings.max_keepalives);
    evhtp_bind_socket(htp, settings.ip, settings.port, settings.backlog);

    event_base_loop(evbase, 0);

    evhtp_unbind_socket(htp);
    //evhtp_free(htp);
    event_base_free(evbase);
    free_headers_conf(settings.headers);
    free_access_conf(settings.up_access);
    free_access_conf(settings.down_access);
    free_access_conf(settings.admin_access);
    free(settings.mp_set);

    return 0;
}

