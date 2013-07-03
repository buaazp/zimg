#include "zcommon.h"

int main(int argc, char **argv)
{
    //Read config
    char *conf = "conf/config.ini";
    char strport[10];
    char strmport[10];

    char _mip[128];

    DEBUG_PRINT("Begin to Read Config File[%s]", conf);

    if(getConfKey(conf, "zhttpd", "port", strport) == -1)
    {
        DEBUG_WARNING("Key port read failed. Use 4869 as default.");
        strcpy(strport, "4869");
    }
    else
    {
        DEBUG_PRINT("Get port: %s", strport);
    }
    _port = atoi(strport);

    if(getConfKey(conf, "memcached", "mport", strmport) == -1)
    {
        DEBUG_WARNING("Key mport read failed. Use 11211 as default.");
        strcpy(strmport, "11211");
    }
    else
    {
        DEBUG_PRINT("Get mport: %s", strmport);
    }

    if(getConfKey(conf, "zhttpd", "root-path", _root_path) == -1)
    {
        DEBUG_WARNING("Key root-path read failed. Use ./www as default.");
        strcpy(_root_path, "./www");
    }
    else
    {
        DEBUG_PRINT("Get root_path: %s", _root_path);
    }

    if(getConfKey(conf, "zimg", "img-path", _img_path) == -1)
    {
        DEBUG_WARNING("Key img-path read failed. Use ./img as default.");
        strcpy(_img_path, "./img");
    }
    else
    {
        DEBUG_PRINT("Get img-path: %s", _img_path);
    }

    if(getConfKey(conf, "zlog", "log-path", _log_path) == -1)
    {
        DEBUG_WARNING("Key log-path read failed. Use ./log as default.");
        strcpy(_log_path, "./log");
    }
    else
    {
        DEBUG_PRINT("Get log-path: %s", _log_path);
    }

    if(getConfKey(conf, "memcached", "mip", _mip) == -1)
    {
        DEBUG_WARNING("Key mip read failed. Use 127.0.0.1 as default.");
        strcpy(_log_path, "127.0.0.1");
    }
    else
    {
        DEBUG_PRINT("Get mip: %s", _mip);
    }


    DEBUG_PRINT("%s Read Finished.", conf);

    //init the Path zimg need to use.
    DEBUG_PRINT("Begin to Init the Path zimg Using...");
    if(isDir(_root_path) != 1)
    {
        if(mkDir(_root_path) != 1)
        {
            DEBUG_ERROR("_root_path[%s] Create Failed!", _root_path);
            return -1;
        }
    }

    if(isDir(_img_path) != 1)
    {
        if(mkDir(_img_path) != 1)
        {
            DEBUG_ERROR("_img_path[%s] Create Failed!", _img_path);
            return -1;
        }
    }
    
    if(isDir(_log_path) != 1)
    {
        if(mkDir(_log_path) != 1)
        {
            DEBUG_ERROR("_log_path[%s] Create Failed!", _log_path);
            return -1;
        }
    }
    DEBUG_PRINT("Paths Init Finished.");

    //init memcached connection...
    DEBUG_PRINT("Begin to Init Memcached Connection...");
    _memc= memcached_create(NULL);

    char *mserver = (char *)malloc(strlen(_mip) + strlen(strmport) + 2);
    sprintf(mserver, "%s:%s", _mip, strmport);
    memcached_server_st *servers = memcached_servers_parse(mserver);

    memcached_server_push(_memc, servers);
    memcached_server_list_free(servers);
    memcached_behavior_set(_memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 0);
    DEBUG_PRINT("Memcached Connection Init Finished.");
    DEBUG_PRINT("memcached connection to: %s", mserver);
    free(mserver);

    //begin to start httpd...
    DEBUG_PRINT("Begin to Start Httpd Server...");
    if(startHttpd(_port, _root_path) == -1)
    {
        DEBUG_ERROR("zhttpd start failed.");
    }

    memcached_free(_memc);
    return 0;
}
