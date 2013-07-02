#include "zutil.h"

int getType(const char *filename, char *type)
{
    char *flag, *tmp;
    if((flag = strchr(filename, '.')) == 0)
    {
        DEBUG_ERROR("FileName [%s] Has No '.' in It.", filename);
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
        DEBUG_PRINT("compare %s - %s.", lower, imgType[i]);
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
        DEBUG_ERROR("Path[%s] is Not Existed!", path);
        return -1;
    }
    if(S_ISDIR(st.st_mode))
    {
        DEBUG_PRINT("Path[%s] is A Dir.", path);
        return 1;
    }
    else
        return -1;
}

int mkDir(const char *path)
{
    if(access(path, 0) == -1)
    {
        DEBUG_PRINT("Begin to mkDir()...");
        int status = mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        if(status == -1)
        {
            DEBUG_ERROR("mkDir Failed!");
            return -1;
        }
        DEBUG_PRINT("mkDir sucessfully!");
        return 1;
    }
    else
    {
        DEBUG_ERROR("Path[%s] is Existed!", path);
        return -1;
    }
}
