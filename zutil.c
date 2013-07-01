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
