#include "zcommon.h"

int exist_cache(const char *key)
{
    int rst = -1;

    size_t valueLen;
    uint32_t  flags;
    memcached_return rc;
//    const char *str_zero = "0";
//    uint64_t ut_zero = 0;

    //rc = memcached_cas(_memc, key, strlen(key), str_zero, strlen(str_zero), 0, flags, 0);
    memcached_get(_memc, key, strlen(key), &valueLen, &flags, &rc);

    if (rc == MEMCACHED_SUCCESS) 
    {
        LOG_PRINT(LOG_INFO, "Cache Key[%s] Exist.", key);
        rst = 1;
    }
//    else if (rc == MEMCACHED_NOTFOUND)
//    {
//        LOG_PRINT(LOG_WARNING, "Cache Key[%s] Not Exist!", key);
//        rst = -1;
//    }
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
