#include "zcommon.h"

int main(int argc, char **argv)
{
    //Read config
    char *conf = "conf/config.ini";
    char strport[10];
    DEBUG_PRINT("Begin to Read Config File[%s]", conf);

    extern int _port;
    extern int _log;

    if(getConfKey(conf, "zhttpd", "port", strport) == -1)
    {
        DEBUG_WARNING("Key port read failed. Use 8080 as default.");
        strcpy(strport, "8080");
    }
    else
    {
        DEBUG_PRINT("Get port: %s", strport);
    }
    _port = atoi(strport);

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

    //begin to start httpd...
    DEBUG_PRINT("Begin to Start Httpd Server...");
    if(startHttpd(_port, _root_path) == -1)
    {
        DEBUG_ERROR("zhttpd start failed.");
    }

    return 0;
}
