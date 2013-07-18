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
 * @file zconf.c
 * @brief Config read functions.
 * @author 招牌疯子 zp@buaa.us
 * @version 1.0
 * @date 2013-07-19
 */


#include "zconf.h"

int get_conf_key(char *conf, char *module, char *key, char *value);
int check_conf(char *conf);


int get_conf_key(char *conf, char *module, char *key, char *value)
{
    if(check_conf(conf) == -1)
    {
        return -1;
    }
    char buf[MAX_LINE];
    FILE *fp = fopen(conf, "r");
    char *p;
    int mflag = 0, kflag = 0;

    if(fp == NULL)
    {
        LOG_PRINT(LOG_ERROR, "Fail to open config file [%s].", conf);
        return -1;
    }
    while(fgets(buf, 100, fp) != NULL)
    {
        buf[strlen(buf) - 1]='\0';
        if(module != NULL)
        {
            if(strchr(buf, '[') == buf && strstr(buf, module) == buf+1 && strchr(buf, ']') == buf+1+strlen(module))
            {
                mflag = 1;
                while(fgets(buf, 100, fp) != NULL)
                {
                    buf[strlen(buf) - 1]='\0';
                    if(strchr(buf, '[') == buf)
                    {
                        LOG_PRINT(LOG_INFO, "Can't find key [%s] in the module [%s].", key, module);
                        fclose(fp);
                        return -1;
                    }
                    else if(strstr(buf, key) == buf && strchr(buf, '=') == buf+strlen(key))
                    {
                        p = buf + strlen(key) + 1;
                        while(isspace(p[0]))
                        {
                            p++;
                        }
                        strcpy(value, p);
                        fclose(fp);
                        LOG_PRINT(LOG_INFO, "Key [%s] = %s", key, value);
                        return 0;
                    }
                }
                LOG_PRINT(LOG_INFO, "Key [%s] Not Found.", key);
                fclose(fp);
                return -1;
            }
        }
        else
        {
            if(strstr(buf, key) == buf && strchr(buf, '=') == buf+strlen(key))
            {
                p = buf + strlen(key) + 1;
                while(isspace(p[0]))
                {
                    p++;
                }
                strcpy(value, p);
                fclose(fp);
                LOG_PRINT(LOG_INFO, "Key [%s] = %s", key, value);
                return 0;
            }
        }
    }
    if(mflag == 0)
    {
        LOG_PRINT(LOG_INFO, "Can't find module [%s] in config file.", module);
    }
    if(kflag == 0)
    {
        LOG_PRINT(LOG_INFO, "Key [%s] Not Found.", key);
    }
    fclose(fp);
    return -1;
}

int check_conf(char *conf)
{
    if(access(conf, F_OK) == 0)
    {
        return 1;
    }
    else
    {
        LOG_PRINT(LOG_ERROR, "%s is not exist.\n", conf);
        return -1;
    }
}
