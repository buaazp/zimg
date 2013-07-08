#include "zcommon.h"

int find_cache(const char *key, char *value)
{
    int rst = -1;

    size_t valueLen;
    uint32_t  flags;
    memcached_return rc;

    char *pvalue = memcached_get(_memc, key, strlen(key), &valueLen, &flags, &rc);

    if (rc == MEMCACHED_SUCCESS) 
    {
        LOG_PRINT(LOG_INFO, "Cache Find Key[%s]: %s", key, pvalue);
        strcpy(value, pvalue);
        free(pvalue);
        rst = 1;
    }
    else if (rc == MEMCACHED_NOTFOUND)
    {
        LOG_PRINT(LOG_WARNING, "Cache Key[%s] Not Find!", key);
        rst = -1;
    }

    return rst;
}

int set_cache(const char *key, const char *value)
{
    int rst = -1;

    uint32_t  flags;
    memcached_return rc;

    rc = memcached_set(_memc, key, strlen(key), value, strlen(value), 0, flags);

    if (rc == MEMCACHED_SUCCESS) 
    {
        LOG_PRINT(LOG_INFO, "Cache Set Successfully. Key[%s]: %s", key, value);
        rst = 1;
    }
    else
    {
        LOG_PRINT(LOG_WARNING, "Cache Set(Key: %s Value: %s) Failed!", key, value);
        rst = -1;
    }

    return rst;
}

int del_cache(const char *key)
{
    int rst = -1;

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
        rst = -1;
    }

    return rst;
}
