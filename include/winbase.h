#ifndef _WINBASE_H
#define _WINBASE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "winnt.h"
#include "winerror.h"

/* Handle definition */
typedef PVOID HANDLE;
typedef HANDLE* PHANDLE;
typedef HANDLE* LPHANDLE;

#define INVALID_HANDLE_VALUE ((HANDLE)(LONG)-1)

/* Reserved values */
#define REG_OPTION_RESERVED         0x00000000L
#define REG_OPTION_NON_VOLATILE     0x00000000L
#define REG_OPTION_VOLATILE         0x00000001L
#define REG_OPTION_CREATE_LINK      0x00000002L
#define REG_OPTION_BACKUP_RESTORE   0x00000004L
#define REG_OPTION_OPEN_LINK        0x00000008L

#ifdef __cplusplus
}
#endif

#endif /* _WINBASE_H */
