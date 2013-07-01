#include "zutil.h"

int getType(const char *filename, char *type)
{
    char *flag;
    if((flag = strchr(filename, '.')) == 0)
    {
        DEBUG_ERROR("FileName [%s] Has No '.' in It.", filename);
        return -1;
    }
    flag++;
    sprintf(type, "%s", flag);
    return 1;
}
