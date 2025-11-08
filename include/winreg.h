#ifndef _WINREG_H
#define _WINREG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "winbase.h"

/* Registry Handle Types */
typedef HANDLE HKEY;
typedef HKEY* PHKEY;

/* Predefined Registry Keys */
#define HKEY_CLASSES_ROOT           ((HKEY)(ULONG_PTR)((LONG)0x80000000))
#define HKEY_CURRENT_USER           ((HKEY)(ULONG_PTR)((LONG)0x80000001))
#define HKEY_LOCAL_MACHINE          ((HKEY)(ULONG_PTR)((LONG)0x80000002))
#define HKEY_USERS                  ((HKEY)(ULONG_PTR)((LONG)0x80000003))
#define HKEY_PERFORMANCE_DATA       ((HKEY)(ULONG_PTR)((LONG)0x80000004))
#define HKEY_CURRENT_CONFIG         ((HKEY)(ULONG_PTR)((LONG)0x80000005))
#define HKEY_DYN_DATA               ((HKEY)(ULONG_PTR)((LONG)0x80000006))

/* Registry Value Types */
#define REG_NONE                    0
#define REG_SZ                      1
#define REG_EXPAND_SZ               2
#define REG_BINARY                  3
#define REG_DWORD                   4
#define REG_DWORD_LITTLE_ENDIAN     4
#define REG_DWORD_BIG_ENDIAN        5
#define REG_LINK                    6
#define REG_MULTI_SZ                7
#define REG_RESOURCE_LIST           8
#define REG_FULL_RESOURCE_DESCRIPTOR 9
#define REG_RESOURCE_REQUIREMENTS_LIST 10
#define REG_QWORD                   11
#define REG_QWORD_LITTLE_ENDIAN     11

/* Registry Access Rights */
#define KEY_QUERY_VALUE             0x0001
#define KEY_SET_VALUE               0x0002
#define KEY_CREATE_SUB_KEY          0x0004
#define KEY_ENUMERATE_SUB_KEYS      0x0008
#define KEY_NOTIFY                  0x0010
#define KEY_CREATE_LINK             0x0020
#define KEY_WOW64_64KEY             0x0100
#define KEY_WOW64_32KEY             0x0200

#define KEY_READ                    ((STANDARD_RIGHTS_READ | \
                                      KEY_QUERY_VALUE | \
                                      KEY_ENUMERATE_SUB_KEYS | \
                                      KEY_NOTIFY) & \
                                     (~SYNCHRONIZE))

#define KEY_WRITE                   ((STANDARD_RIGHTS_WRITE | \
                                      KEY_SET_VALUE | \
                                      KEY_CREATE_SUB_KEY) & \
                                     (~SYNCHRONIZE))

#define KEY_EXECUTE                 ((KEY_READ) & (~SYNCHRONIZE))

#define KEY_ALL_ACCESS              ((STANDARD_RIGHTS_ALL | \
                                      KEY_QUERY_VALUE | \
                                      KEY_SET_VALUE | \
                                      KEY_CREATE_SUB_KEY | \
                                      KEY_ENUMERATE_SUB_KEYS | \
                                      KEY_NOTIFY | \
                                      KEY_CREATE_LINK) & \
                                     (~SYNCHRONIZE))

/* Registry Creation/Open Disposition */
#define REG_CREATED_NEW_KEY         0x00000001L
#define REG_OPENED_EXISTING_KEY     0x00000002L

/* Registry Notify Filter */
#define REG_NOTIFY_CHANGE_NAME      0x00000001L
#define REG_NOTIFY_CHANGE_ATTRIBUTES 0x00000002L
#define REG_NOTIFY_CHANGE_LAST_SET  0x00000004L
#define REG_NOTIFY_CHANGE_SECURITY  0x00000008L

/* Registry API Functions */

/**
 * Opens the specified registry key
 */
LONG RegOpenKeyExA(
    HKEY hKey,
    LPCSTR lpSubKey,
    DWORD ulOptions,
    DWORD samDesired,
    PHKEY phkResult
);

LONG RegOpenKeyExW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD ulOptions,
    DWORD samDesired,
    PHKEY phkResult
);

#ifdef UNICODE
#define RegOpenKeyEx RegOpenKeyExW
#else
#define RegOpenKeyEx RegOpenKeyExA
#endif

/**
 * Opens the specified registry key (simplified version)
 */
LONG RegOpenKeyA(
    HKEY hKey,
    LPCSTR lpSubKey,
    PHKEY phkResult
);

LONG RegOpenKeyW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    PHKEY phkResult
);

#ifdef UNICODE
#define RegOpenKey RegOpenKeyW
#else
#define RegOpenKey RegOpenKeyA
#endif

/**
 * Creates the specified registry key
 */
LONG RegCreateKeyExA(
    HKEY hKey,
    LPCSTR lpSubKey,
    DWORD Reserved,
    LPSTR lpClass,
    DWORD dwOptions,
    DWORD samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,
    LPDWORD lpdwDisposition
);

LONG RegCreateKeyExW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    DWORD Reserved,
    LPWSTR lpClass,
    DWORD dwOptions,
    DWORD samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PHKEY phkResult,
    LPDWORD lpdwDisposition
);

#ifdef UNICODE
#define RegCreateKeyEx RegCreateKeyExW
#else
#define RegCreateKeyEx RegCreateKeyExA
#endif

/**
 * Creates the specified registry key (simplified version)
 */
LONG RegCreateKeyA(
    HKEY hKey,
    LPCSTR lpSubKey,
    PHKEY phkResult
);

LONG RegCreateKeyW(
    HKEY hKey,
    LPCWSTR lpSubKey,
    PHKEY phkResult
);

#ifdef UNICODE
#define RegCreateKey RegCreateKeyW
#else
#define RegCreateKey RegCreateKeyA
#endif

/**
 * Closes a handle to the specified registry key
 */
LONG RegCloseKey(
    HKEY hKey
);

/**
 * Retrieves the type and data for the specified value name
 */
LONG RegQueryValueExA(
    HKEY hKey,
    LPCSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
);

LONG RegQueryValueExW(
    HKEY hKey,
    LPCWSTR lpValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
);

#ifdef UNICODE
#define RegQueryValueEx RegQueryValueExW
#else
#define RegQueryValueEx RegQueryValueExA
#endif

/**
 * Sets the data and type of a specified value under a registry key
 */
LONG RegSetValueExA(
    HKEY hKey,
    LPCSTR lpValueName,
    DWORD Reserved,
    DWORD dwType,
    const BYTE* lpData,
    DWORD cbData
);

LONG RegSetValueExW(
    HKEY hKey,
    LPCWSTR lpValueName,
    DWORD Reserved,
    DWORD dwType,
    const BYTE* lpData,
    DWORD cbData
);

#ifdef UNICODE
#define RegSetValueEx RegSetValueExW
#else
#define RegSetValueEx RegSetValueExA
#endif

/**
 * Deletes a subkey and its values
 */
LONG RegDeleteKeyA(
    HKEY hKey,
    LPCSTR lpSubKey
);

LONG RegDeleteKeyW(
    HKEY hKey,
    LPCWSTR lpSubKey
);

#ifdef UNICODE
#define RegDeleteKey RegDeleteKeyW
#else
#define RegDeleteKey RegDeleteKeyA
#endif

/**
 * Removes a named value from the specified registry key
 */
LONG RegDeleteValueA(
    HKEY hKey,
    LPCSTR lpValueName
);

LONG RegDeleteValueW(
    HKEY hKey,
    LPCWSTR lpValueName
);

#ifdef UNICODE
#define RegDeleteValue RegDeleteValueW
#else
#define RegDeleteValue RegDeleteValueA
#endif

/**
 * Enumerates the subkeys of the specified open registry key
 */
LONG RegEnumKeyExA(
    HKEY hKey,
    DWORD dwIndex,
    LPSTR lpName,
    LPDWORD lpcchName,
    LPDWORD lpReserved,
    LPSTR lpClass,
    LPDWORD lpcchClass,
    PFILETIME lpftLastWriteTime
);

LONG RegEnumKeyExW(
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpName,
    LPDWORD lpcchName,
    LPDWORD lpReserved,
    LPWSTR lpClass,
    LPDWORD lpcchClass,
    PFILETIME lpftLastWriteTime
);

#ifdef UNICODE
#define RegEnumKeyEx RegEnumKeyExW
#else
#define RegEnumKeyEx RegEnumKeyExA
#endif

/**
 * Enumerates the values for the specified open registry key
 */
LONG RegEnumValueA(
    HKEY hKey,
    DWORD dwIndex,
    LPSTR lpValueName,
    LPDWORD lpcchValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
);

LONG RegEnumValueW(
    HKEY hKey,
    DWORD dwIndex,
    LPWSTR lpValueName,
    LPDWORD lpcchValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
);

#ifdef UNICODE
#define RegEnumValue RegEnumValueW
#else
#define RegEnumValue RegEnumValueA
#endif

/**
 * Retrieves information about the specified registry key
 */
LONG RegQueryInfoKeyA(
    HKEY hKey,
    LPSTR lpClass,
    LPDWORD lpcchClass,
    LPDWORD lpReserved,
    LPDWORD lpcSubKeys,
    LPDWORD lpcbMaxSubKeyLen,
    LPDWORD lpcbMaxClassLen,
    LPDWORD lpcValues,
    LPDWORD lpcbMaxValueNameLen,
    LPDWORD lpcbMaxValueLen,
    LPDWORD lpcbSecurityDescriptor,
    PFILETIME lpftLastWriteTime
);

LONG RegQueryInfoKeyW(
    HKEY hKey,
    LPWSTR lpClass,
    LPDWORD lpcchClass,
    LPDWORD lpReserved,
    LPDWORD lpcSubKeys,
    LPDWORD lpcbMaxSubKeyLen,
    LPDWORD lpcbMaxClassLen,
    LPDWORD lpcValues,
    LPDWORD lpcbMaxValueNameLen,
    LPDWORD lpcbMaxValueLen,
    LPDWORD lpcbSecurityDescriptor,
    PFILETIME lpftLastWriteTime
);

#ifdef UNICODE
#define RegQueryInfoKey RegQueryInfoKeyW
#else
#define RegQueryInfoKey RegQueryInfoKeyA
#endif

/**
 * Deletes the subkeys and values of the specified key recursively
 */
LONG RegDeleteTreeA(
    HKEY hKey,
    LPCSTR lpSubKey
);

LONG RegDeleteTreeW(
    HKEY hKey,
    LPCWSTR lpSubKey
);

#ifdef UNICODE
#define RegDeleteTree RegDeleteTreeW
#else
#define RegDeleteTree RegDeleteTreeA
#endif

/**
 * Flushes the attributes of the specified open registry key to disk
 */
LONG RegFlushKey(
    HKEY hKey
);

#ifdef __cplusplus
}
#endif

#endif /* _WINREG_H */
