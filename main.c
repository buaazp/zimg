#include "zcommon.h"

int main(int argc, char **argv)
{
    char *conf = "conf/config.ini";
    char strport[10];

    extern int _port;
    extern int _log;

    if(getConfKey(conf, "zhttpd", "port", strport) == -1)
    {
        DEBUG_PRINT("Key port read failed. Use 8080 as default.");
        strcpy(strport, "8080");
    }
    else
    {
        DEBUG_PRINT("Get port: %s", strport);
    }
    _port = atoi(strport);

    if(getConfKey(conf, "zhttpd", "root-path", _root_path) == -1)
    {
        DEBUG_PRINT("Key root-path read failed. Use ./www as default.");
        strcpy(_root_path, "./www");
    }
    else
    {
        DEBUG_PRINT("Get root_path: %s", _root_path);
    }

    if(getConfKey(conf, "zimg", "img-path", _img_path) == -1)
    {
        DEBUG_PRINT("Key img-path read failed. Use ./img as default.");
        strcpy(_img_path, "./img");
    }
    else
    {
        DEBUG_PRINT("Get img-path: %s", _img_path);
    }

    if(startHttpd(_port, _root_path) == -1)
    {
        DEBUG_ERROR("zhttpd start failed.");
    }

    return 0;
}
