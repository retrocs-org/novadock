#ifndef ND_WHDLOAD_STATUS_H
#define ND_WHDLOAD_STATUS_H
#include <string.h>
#define ND_WHD_FOLDER_OK 0
#define ND_WHD_FOLDER_UNSET 1
#define ND_WHD_FOLDER_MISSING 2
#define ND_WHD_FOLDER_NOT_DRAWER 3
#define ND_WHD_FOLDER_UNREADABLE 4
#define ND_WHD_FOLDER_MEMORY 5
#define ND_WHD_FOLDER_UNAVAILABLE 6
static const char *nd_whd_folder_value(int status)
{
    static const char *const values[]={"","NO DRAWER SELECTED","DRAWER NOT FOUND","NOT A DRAWER","CANNOT READ DRAWER","OUT OF MEMORY","VOLUME UNAVAILABLE"};
    return values[status>=0 && status<=6 ? status:ND_WHD_FOLDER_UNREADABLE];
}
static int nd_whd_folder_code(const char *value)
{
    int i;if(!value)return ND_WHD_FOLDER_OK;
    for(i=1;i<=6;++i)if(!strcmp(value,nd_whd_folder_value(i)))return i;
    return !strcmp(value,"CHECK FOLDER") ? ND_WHD_FOLDER_UNREADABLE:ND_WHD_FOLDER_OK;
}
#endif
