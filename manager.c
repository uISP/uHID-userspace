#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <hidapi/hidapi.h>
#include <libuhid.h>

#ifdef _WIN32
#include <malloc.h>	/* for alloca() */
#include <Shlobj.h>
#endif


char *uhidmgrGetAppHomeDir(const char *subdir)
{
    char *ret;
    int len;
    if (!subdir)
        subdir="";

#ifdef _WIN32
#define FMT "%s\\uHID\\%s"
    char path[MAX_PATH];
    SHGetFolderPath(NULL, CSIDL_APPDATA , 0, (DWORD) NULL, path);
#else
#define FMT "%s/.uHID/%s"
    char *path = getenv("HOME");
    if (-1 == asprintf(&ret, FMT, path, subdir))
        return NULL;
#endif

    len = snprintf(NULL, 0, FMT, path, subdir) + 1;
    ret = malloc(len);
    if (!ret)
        return NULL;
    sprintf(ret, FMT, path, subdir);

    if (!subdir[0]) {
        int status = mkdir(ret, 0755);
        if (status == 0){
            printf("Created appdata directory\n");
        }
    }

    return ret;
}
