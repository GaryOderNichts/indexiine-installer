#ifndef _MAIN_H_
#define _MAIN_H_

#include "common/types.h"
#include "common/common.h"
#include "dynamic_libs/os_functions.h"

#define MLC_MOUNT_PATH "/vol/storage_mlc01"

#define BROWSER_PATH_EUR "dev:/sys/title/00050030/1001220a"
#define BROWSER_PATH_USA "dev:/sys/title/00050030/1001210a"
#define BROWSER_PATH_JPN "dev:/sys/title/00050030/1001200a"

#define INDEX_PATH "/content/pages/index.html"

#define INDEX_BACKUP_PATH "sd:/wiiu/apps/indexiine-installer/backup-index.html"
#define INDEXIINE_INDEX_PATH "sd:/wiiu/apps/indexiine-installer/index.html"

#define INDEX_MODE 0x644

/* Main */
#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    Undetected,
    EUR,
    USA,
    JPN
} Region;


int Menu_Main(void);
void console_printf(int newline, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif
