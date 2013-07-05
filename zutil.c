#include "zutil.h"

// this data is for KMP searching
static int pi[128];

/* KMP for searching */
static void kmp_init(const unsigned char *pattern, int pattern_size)  // prefix-function
{
    pi[0] = 0;  // pi[0] always equals to 0 by defination
    int k = 0;  // an important pointer
    int q;
    for(q = 1; q < pattern_size; q++)  // find each pi[q] for pattern[q]
    {
        while(k>0 && pattern[k+1]!=pattern[q])
            k = pi[k];  // use previous prefixes to match pattern[0..q]

        if(pattern[k+1] == pattern[q]) // if pattern[0..(k+1)] is a prefix
            k++;             // let k = k + 1

        pi[q] = k;   // be ware, (0 <= k <= q), and (pi[k] < k)
    }
    // The worst-case time complexity of this procedure is O(pattern_size)
}

int kmp(const unsigned char *matcher, int mlen, const unsigned char *pattern, int plen)
{
    // this function does string matching using the KMP algothm.
    // matcher and pattern are pointers to BINARY sequencies, while mlen
    // and plen are their lengths respectively.
    // if a match is found, return its position immediately.
    // return -1 if no match can be found.

    if(!mlen || !plen || mlen < plen) // take care of illegal parameters
        return -1;

    kmp_init(pattern, plen);  // prefix-function

    int i=0, j=0;
    while(i < mlen && j < plen)  // don't increase i and j at this level
    {
        if(matcher[i+j] == pattern[j])
            j++;
        else if(j == 0)  // dismatch: matcher[i] vs pattern[0]
            i++;
        else      // dismatch: matcher[i+j] vs pattern[j], and j>0
        {
            i = i + j - pi[j-1];  // i: jump forward by (j - pi[j-1])
            j = pi[j-1];          // j: reset to the proper position
        }
    }
    if(j == plen) // found a match!!
    {
        return i;
    }
    else          // if no match was found
        return -1;
}

int getType(const char *filename, char *type)
{
    char *flag, *tmp;
    if((flag = strchr(filename, '.')) == 0)
    {
        LOG_PRINT(LOG_ERROR, "FileName [%s] Has No '.' in It.", filename);
        return -1;
    }
    while((tmp = strchr(flag + 1, '.')) != 0)
    {
        flag = tmp;
    }
    flag++;
    sprintf(type, "%s", flag);
    return 1;
}

int isImg(const char *filename)
{
    char *imgType[] = {"jpg", "jpeg", "png", "gif"};
    char *lower= (char *)malloc(strlen(filename) + 1);
    char *tmp;
    int i;
    int isimg = 0;
    for(i = 0; i < strlen(filename); i++)
    {
        lower[i] = tolower(filename[i]);
    }
    for(i = 0; i < 4; i++)
    {
        LOG_PRINT(LOG_INFO, "compare %s - %s.", lower, imgType[i]);
        if((tmp = strstr(lower, imgType[i])) == lower)
        {
            isimg = 1;
            break;
        }
    }
    return isimg;
}

int isDir(const char *path)
{
    struct stat st;
    if(stat(path, &st)<0)
    {
        if(_log_id == -1)
            DEBUG_ERROR("Path[%s] is Not Existed!", path);
        else
            LOG_PRINT(LOG_ERROR, "Path[%s] is Not Existed!", path);
        return -1;
    }
    if(S_ISDIR(st.st_mode))
    {
        if(_log_id == -1)
            DEBUG_PRINT("Path[%s] is A Dir.", path);
        else
            LOG_PRINT(LOG_INFO, "Path[%s] is A Dir.", path);
        return 1;
    }
    else
        return -1;
}

int mkDir(const char *path)
{
    if(access(path, 0) == -1)
    {
        if(_log_id == -1)
            DEBUG_PRINT("Begin to mkDir()...");
        else
            LOG_PRINT(LOG_INFO, "Begin to mkDir()...");
        int status = mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if(status == -1)
        {
            if(_log_id == -1)
                DEBUG_ERROR("mkDir[%s] Failed!", path);
            else
                LOG_PRINT(LOG_ERROR, "mkDir[%s] Failed!", path);
            return -1;
        }
        if(_log_id == -1)
            DEBUG_PRINT("mkDir[%s] sucessfully!", path);
        else
            LOG_PRINT(LOG_INFO, "mkDir[%s] sucessfully!", path);
        return 1;
    }
    else
    {
        if(_log_id == -1)
            DEBUG_ERROR("Path[%s] is Existed!", path);
        else
            LOG_PRINT(LOG_ERROR, "Path[%s] is Existed!", path);
        return -1;
    }
}
