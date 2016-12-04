/*
 *  uHID Universal MCU Bootloader. Library.
 *  Copyright (C) 2016  Andrew 'Necromant' Andrianov
 *
 *  This file is part of uHID project. uHID was initially based
 *  on bootloadHID avr bootloader by Christian Starkjohann
 *  Since no original userspace code remains, all userspace code
 *  is now LGPLv2.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.

 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.

 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

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
#include <dirent.h>


#ifdef _WIN32
#include <malloc.h>	/* for alloca() */
#include <Shlobj.h>
#endif


static void replace_char(char *str, int c, int repl)
{
    char *pos;
    while (1) {
        pos = strchr(str, c);
        if (!pos)
            break;
        *pos = repl;
    }
}

static int do_mkdir(const char* path, mode_t mode)
{
    #ifdef _WIN32
    return mkdir(path);
    #else
    return mkdir(path, mode);
    #endif
}

static int mkpath(const char* path, mode_t mode) {
  char *file_path = strdup(path);
  int ret = -1;
  if (!file_path)
    return ret;

  char* p;
  for (p=strchr(file_path+1, '/'); p; p=strchr(p+1, '/')) {
    *p='\0';
    if (do_mkdir(file_path, mode)==-1) {
      if (errno!=EEXIST)
        goto bailout;;
    }
    *p='/';
  }

  ret = 0;
bailout:
  free(file_path);
  return ret;
}

static void replace_set(char *str, char *set, int repl)
{
    while (*set)
        replace_char(str, *set++, repl);
}

UHID_API char *uhidmgrGetAppHomeDir(const char *subdir)
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
#endif

    len = snprintf(NULL, 0, FMT, path, subdir) + 1;
    ret = malloc(len);
    if (!ret)
        return NULL;
    sprintf(ret, FMT, path, subdir);

/* Fix any paths */
#ifdef _WIN32
    replace_char(ret, '/', '\\');
#else
    replace_char(ret, '\\', '/');
#endif

    /* ditch forbidden chars */
    replace_set(ret, "!@#$%^&*(),?", '_');

    if (!subdir[0]) {
        int status = mkpath(ret, 0755);
        if (status == 0){
            printf("Created appdata directory: %s\n", ret);
        }
    }

#undef FMT
    return ret;
}


UHID_API char *uhidmgrAppDir(hid_device *dev, const char *appname)
{
    wchar_t tmp[255];
    int err;
    int len;
    if (!appname)
      appname = "";
#define GET_STR(func, dest) \
    err = func(dev, tmp, 255); \
    if (err) \
        return NULL; \
    len = wcsnlen(tmp, 255) + 1; \
    char *dest = alloca(len); \
    wcstombs(dest, tmp, len);

    GET_STR(hid_get_manufacturer_string, manuf);
    GET_STR(hid_get_product_string, device);
    GET_STR(hid_get_serial_number_string, serial);

    struct uHidDeviceInfo *inf = uhidReadInfo(dev);
    if (!inf)
        return NULL;

    /* TODO: Use VID/PID here */
//    int vid,pid;

    /* Drop separator */
    replace_char(serial, ':', 0x0);
    #define FMT "firmwares/%s/%s/%s/%.01fMhz/%s"

    len = snprintf(NULL, 0, FMT,
        manuf, device, serial,
        uhidGetFrequencyMhz(inf), appname) + 1;

    char *ret = alloca(len);
    snprintf(ret, len, FMT,
      manuf, device, serial,
      uhidGetFrequencyMhz(inf), appname);

    ret = uhidmgrGetAppHomeDir(ret);
    free(inf);
    return ret;
}

UHID_API struct uhidApplication *uhidmgrRepoRead(hid_device *dev)
{
  char *path = uhidmgrAppDir(dev, NULL);
  printf("Repo path: %s\n", path);
  mkpath(path, 0755);
  DIR *dir = opendir(path);
  if (!dir)
    return NULL;

  do {
  struct dirent *entry = readdir(dir);
  if (!entry)
    return NULL;
  } while (0);

  return NULL;
}

UHID_API struct uhidApplication *uhidmgrRepoReadNext(struct uhidApplication *app)
{

}

UHID_API struct uhidApplication *uhidmgrRepoFind(struct uhidApplication *app)
{

}

UHID_API int uhidmgrAppLoad(struct uhidApplication *app)
{

}
