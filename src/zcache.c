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


#include "zcache.h"
#include "zlog.h"

extern struct setting settings;

int exist_cache(memcached_st *memc, const char *key);
int find_cache(memcached_st *memc, const char *key, char *value);
int set_cache(memcached_st *memc, const char *key, const char *value);
int find_cache_bin(memcached_st *memc, const char *key, char **value_ptr, size_t *len);
int set_cache_bin(memcached_st *memc, const char *key, const char *value, const size_t len);
int del_cache(memcached_st *memc, const char *key);

/**
 * @brief exist_cache Check a key is exist in memcached.
 *
 * @param memc The connection to beansdb.
 * @param key The string of the key.
 *
 * @return 1 for yes and -1 for no.
 */
int exist_cache(memcached_st *memc, const char *key)
{
    int rst = -1;
    if(settings.cache_on == false)
        return rst;

    size_t valueLen;
    uint32_t  flags;
    memcached_return rc;

    memcached_get(memc, key, strlen(key), &valueLen, &flags, &rc);

    if (rc == MEMCACHED_SUCCESS) 
    {
        LOG_PRINT(LOG_DEBUG, "Cache Key[%s] Exist.", key);
        rst = 1;
    }
    else
    {
        const char *str_rc = memcached_strerror(memc, rc);
        LOG_PRINT(LOG_DEBUG, "Cache Result: %s", str_rc);
    }

    return rst;
}

/**
 * @brief find_cache Connect to a memcached server and find a key's value.
 *
 * @param memc The connection to beansdb.
 * @param key The string of the key you want to find.
 * @param value It contains the string of the key's value.
 *
 * @return 1 for success and -1 for fail.
 */
int find_cache(memcached_st *memc, const char *key, char *value)
{
    int rst = -1;
    if(settings.cache_on == false)
        return rst;

    size_t valueLen;
    uint32_t  flags;
    memcached_return rc;

    char *pvalue = memcached_get(memc, key, strlen(key), &valueLen, &flags, &rc);

    if (rc == MEMCACHED_SUCCESS) 
    {
        LOG_PRINT(LOG_DEBUG, "Cache Find Key[%s] Value: %s", key, pvalue);
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
        const char *str_rc = memcached_strerror(memc, rc);
        LOG_PRINT(LOG_DEBUG, "Cache Result: %s", str_rc);
    }

    return rst;
}

/**
 * @brief set_cache Set a key with the value input.
 *
 * @param memc The connection to beansdb.
 * @param key The key you want to set a new value.
 * @param value The value of the key.
 *
 * @return 1 for success and -1 for fail.
 */
int set_cache(memcached_st *memc, const char *key, const char *value)
{
    int rst = -1;
    if(settings.cache_on == false)
        return rst;

    memcached_return rc;

    rc = memcached_set(memc, key, strlen(key), value, strlen(value), 0, 0);

    if (rc == MEMCACHED_SUCCESS) 
    {
        LOG_PRINT(LOG_DEBUG, "Cache Set Successfully. Key[%s] Value: %s", key, value);
        rst = 1;
    }
    else
    {
        LOG_PRINT(LOG_WARNING, "Cache Set(Key: %s Value: %s) Failed!", key, value);
        const char *str_rc = memcached_strerror(memc, rc);
        LOG_PRINT(LOG_DEBUG, "Cache Result: %s", str_rc);
        rst = -1;
    }

    return rst;
}

/**
 * @brief find_cache_bin Find a key's BINARY value.
 *
 * @param memc The connection to beansdb.
 * @param key The key you want to find.
 * @param value_ptr It will be alloc and contains the binary value.
 * @param len It will change to the length of the value.
 *
 * @return 1 for success and -1 for fail.
 */
//int find_cache_bin(const char *key, char **value_ptr, size_t *len)
int find_cache_bin(memcached_st *memc, const char *key, char **value_ptr, size_t *len)
{
    int rst = -1;
    if(settings.cache_on == false)
        return rst;

    uint32_t  flags;
    memcached_return rc;

    *value_ptr = memcached_get(memc, key, strlen(key), len, &flags, &rc);

    if (rc == MEMCACHED_SUCCESS) 
    {
        LOG_PRINT(LOG_DEBUG, "Binary Cache Find Key[%s], Len: %d.", key, *len);
        rst = 1;
    }
    else if (rc == MEMCACHED_NOTFOUND)
    {
        LOG_PRINT(LOG_WARNING, "Binary Cache Key[%s] Not Find!", key);
        rst = -1;
    }
    else
    {
        const char *str_rc = memcached_strerror(memc, rc);
        LOG_PRINT(LOG_DEBUG, "Cache Result: %s", str_rc);
    }

    //memcached_free(memc);
    return rst;
}

/**
 * @brief set_cache_bin Set a new BINARY value of a key.
 *
 * @param memc The connection to beansdb.
 * @param key The key.
 * @param value A char * buffer you want to set.
 * @param len The length of the buffer above,
 *
 * @return  1 for success and -1 for fial.
 */
int set_cache_bin(memcached_st *memc, const char *key, const char *value, const size_t len)
{
    int rst = -1;
    if(settings.cache_on == false)
        return rst;

    memcached_return rc;

    rc = memcached_set(memc, key, strlen(key), value, len, 0, 0);

    if (rc == MEMCACHED_SUCCESS) 
    {
        LOG_PRINT(LOG_DEBUG, "Binary Cache Set Successfully. Key[%s] Len: %d.", key, len);
        rst = 1;
		settings.cache_on = true;
    }
    else
    {
        LOG_PRINT(LOG_WARNING, "Binary Cache Set(Key: %s ) Failed!", key);
        const char *str_rc = memcached_strerror(memc, rc);
        LOG_PRINT(LOG_DEBUG, "Cache Result: %s", str_rc);
        rst = -1;
		settings.cache_on = false;
    }

    return rst;
}


/**
 * @brief del_cache This function delete a key and its value in memcached.
 *
 * @param memc The connection to beansdb.
 * @param key The key.
 *
 * @return  1 for success and -1 for fail.
 */
int del_cache(memcached_st *memc, const char *key)
{
    int rst = -1;
    if(settings.cache_on == false)
        return rst;

    memcached_return rc;

    rc = memcached_delete(memc, key, strlen(key), 0);

    if (rc == MEMCACHED_SUCCESS) 
    {
        LOG_PRINT(LOG_DEBUG, "Cache Key[%s] Delete Successfully.", key);
        rst = 1;
    }
    else
    {
        LOG_PRINT(LOG_WARNING, "Cache Key[%s] Delete Failed!", key);
        const char *str_rc = memcached_strerror(memc, rc);
        LOG_PRINT(LOG_DEBUG, "Cache Result: %s", str_rc);
        rst = -1;
    }

    return rst;
}
