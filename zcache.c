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
 * @file zcache.c
 * @brief memcached functions.
 * @author 招牌疯子 zp@buaa.us
 * @version 1.0
 * @date 2013-07-19
 */


#include "zcommon.h"
extern struct setting settings;

int exist_cache(const char *key);
int find_cache(const char *key, char *value);
int set_cache(const char *key, const char *value);
int find_cache_bin(const char *key, char **value_ptr, size_t *len);
int set_cache_bin(const char *key, const char *value, const size_t len);
int del_cache(const char *key);

int exist_cache(const char *key)
{
    int rst = -1;
    if(settings.cache_on == false)
        return rst;

    size_t valueLen;
    uint32_t  flags;
    memcached_return rc;

    memcached_get(_memc, key, strlen(key), &valueLen, &flags, &rc);

    if (rc == MEMCACHED_SUCCESS) 
    {
        LOG_PRINT(LOG_INFO, "Cache Key[%s] Exist.", key);
        rst = 1;
    }
    else
    {
        const char *str_rc = memcached_strerror(_memc, rc);
        LOG_PRINT(LOG_INFO, "Cache Result: %s", str_rc);
    }

    return rst;
}

int find_cache(const char *key, char *value)
{
    int rst = -1;
    if(settings.cache_on == false)
        return rst;

    size_t valueLen;
    uint32_t  flags;
    memcached_return rc;

    char *pvalue = memcached_get(_memc, key, strlen(key), &valueLen, &flags, &rc);

    if (rc == MEMCACHED_SUCCESS) 
    {
        LOG_PRINT(LOG_INFO, "Cache Find Key[%s] Value: %s", key, pvalue);
        strcpy(value, pvalue);
        free(pvalue);
        rst = 1;
    }
    else if (rc == MEMCACHED_NOTFOUND)
    {
        LOG_PRINT(LOG_WARNING, "Cache Key[%s] Not Find!", key);
        rst = -1;
    }
    else
    {
        const char *str_rc = memcached_strerror(_memc, rc);
        LOG_PRINT(LOG_INFO, "Cache Result: %s", str_rc);
    }

    return rst;
}

int set_cache(const char *key, const char *value)
{
    int rst = -1;
    if(settings.cache_on == false)
        return rst;

    uint32_t  flags;
    memcached_return rc;

    rc = memcached_set(_memc, key, strlen(key), value, strlen(value), 0, flags);

    if (rc == MEMCACHED_SUCCESS) 
    {
        LOG_PRINT(LOG_INFO, "Cache Set Successfully. Key[%s] Value: %s", key, value);
        rst = 1;
    }
    else
    {
        LOG_PRINT(LOG_WARNING, "Cache Set(Key: %s Value: %s) Failed!", key, value);
        const char *str_rc = memcached_strerror(_memc, rc);
        LOG_PRINT(LOG_INFO, "Cache Result: %s", str_rc);
        rst = -1;
    }

    return rst;
}

int find_cache_bin(const char *key, char **value_ptr, size_t *len)
{
    int rst = -1;
    if(settings.cache_on == false)
        return rst;

    uint32_t  flags;
    memcached_return rc;

    *value_ptr = memcached_get(_memc, key, strlen(key), len, &flags, &rc);

    if (rc == MEMCACHED_SUCCESS) 
    {
        LOG_PRINT(LOG_INFO, "Binary Cache Find Key[%s], Len: %d.", key, *len);
        rst = 1;
    }
    else if (rc == MEMCACHED_NOTFOUND)
    {
        LOG_PRINT(LOG_WARNING, "Binary Cache Key[%s] Not Find!", key);
        rst = -1;
    }
    else
    {
        const char *str_rc = memcached_strerror(_memc, rc);
        LOG_PRINT(LOG_INFO, "Cache Result: %s", str_rc);
    }

    return rst;
}

int set_cache_bin(const char *key, const char *value, const size_t len)
{
    int rst = -1;
    if(settings.cache_on == false)
        return rst;

    uint32_t  flags;
    memcached_return rc;

    rc = memcached_set(_memc, key, strlen(key), value, len, 0, flags);

    if (rc == MEMCACHED_SUCCESS) 
    {
        LOG_PRINT(LOG_INFO, "Binary Cache Set Successfully. Key[%s] Len: %d.", key, len);
        rst = 1;
    }
    else
    {
        LOG_PRINT(LOG_WARNING, "Binary Cache Set(Key: %s ) Failed!", key);
        const char *str_rc = memcached_strerror(_memc, rc);
        LOG_PRINT(LOG_INFO, "Cache Result: %s", str_rc);
        rst = -1;
    }

    return rst;
}


int del_cache(const char *key)
{
    int rst = -1;
    if(settings.cache_on == false)
        return rst;

    memcached_return rc;

    rc = memcached_delete(_memc, key, strlen(key), 0);

    if (rc == MEMCACHED_SUCCESS) 
    {
        LOG_PRINT(LOG_INFO, "Cache Key[%s] Delete Successfully.", key);
        rst = 1;
    }
    else
    {
        LOG_PRINT(LOG_WARNING, "Cache Key[%s] Delete Failed!", key);
        const char *str_rc = memcached_strerror(_memc, rc);
        LOG_PRINT(LOG_INFO, "Cache Result: %s", str_rc);
        rst = -1;
    }

    return rst;
}
