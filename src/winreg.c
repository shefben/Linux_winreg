#include "registry_internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Library initialization flag */
static BOOL g_initialized = FALSE;
static pthread_mutex_t g_init_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Ensure library is initialized */
static LONG ensure_initialized(void) {
    if (g_initialized) {
        return ERROR_SUCCESS;
    }

    pthread_mutex_lock(&g_init_mutex);

    if (!g_initialized) {
        LONG result = db_init();
        if (result == ERROR_SUCCESS) {
            g_initialized = TRUE;
        }
        pthread_mutex_unlock(&g_init_mutex);
        return result;
    }

    pthread_mutex_unlock(&g_init_mutex);
    return ERROR_SUCCESS;
}

/* RegOpenKeyExA */
LONG RegOpenKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, DWORD samDesired, PHKEY phkResult) {
    if (!phkResult) {
        return ERROR_INVALID_PARAMETER;
    }

    LONG result = ensure_initialized();
    if (result != ERROR_SUCCESS) {
        return result;
    }

    char* full_path = build_full_path(hKey, lpSubKey);
    if (!full_path) {
        return ERROR_INVALID_HANDLE;
    }

    sqlite3_int64 key_id;
    result = db_open_key(full_path, &key_id);

    if (result == ERROR_SUCCESS) {
        *phkResult = create_registry_handle(key_id, samDesired, full_path, FALSE);
        if (!*phkResult) {
            result = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    free(full_path);
    return result;
}

/* RegOpenKeyExW */
LONG RegOpenKeyExW(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, DWORD samDesired, PHKEY phkResult) {
    char* sub_key_utf8 = wchar_to_utf8(lpSubKey);
    LONG result = RegOpenKeyExA(hKey, sub_key_utf8, ulOptions, samDesired, phkResult);
    free(sub_key_utf8);
    return result;
}

/* RegOpenKeyA */
LONG RegOpenKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult) {
    return RegOpenKeyExA(hKey, lpSubKey, 0, KEY_READ, phkResult);
}

/* RegOpenKeyW */
LONG RegOpenKeyW(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult) {
    return RegOpenKeyExW(hKey, lpSubKey, 0, KEY_READ, phkResult);
}

/* RegCreateKeyExA */
LONG RegCreateKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD Reserved, LPSTR lpClass,
                     DWORD dwOptions, DWORD samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                     PHKEY phkResult, LPDWORD lpdwDisposition) {
    if (!phkResult) {
        return ERROR_INVALID_PARAMETER;
    }

    LONG result = ensure_initialized();
    if (result != ERROR_SUCCESS) {
        return result;
    }

    char* full_path = build_full_path(hKey, lpSubKey);
    if (!full_path) {
        return ERROR_INVALID_HANDLE;
    }

    sqlite3_int64 key_id;
    DWORD disposition;

    result = db_create_key(full_path, dwOptions, &key_id, &disposition);

    if (result == ERROR_SUCCESS) {
        *phkResult = create_registry_handle(key_id, samDesired, full_path, FALSE);
        if (!*phkResult) {
            result = ERROR_NOT_ENOUGH_MEMORY;
        } else if (lpdwDisposition) {
            *lpdwDisposition = disposition;
        }
    }

    free(full_path);
    return result;
}

/* RegCreateKeyExW */
LONG RegCreateKeyExW(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPWSTR lpClass,
                     DWORD dwOptions, DWORD samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                     PHKEY phkResult, LPDWORD lpdwDisposition) {
    char* sub_key_utf8 = wchar_to_utf8(lpSubKey);
    char* class_utf8 = wchar_to_utf8((const wchar_t*)lpClass);

    LONG result = RegCreateKeyExA(hKey, sub_key_utf8, Reserved, class_utf8, dwOptions,
                                  samDesired, lpSecurityAttributes, phkResult, lpdwDisposition);

    free(sub_key_utf8);
    free(class_utf8);
    return result;
}

/* RegCreateKeyA */
LONG RegCreateKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult) {
    return RegCreateKeyExA(hKey, lpSubKey, 0, NULL, REG_OPTION_NON_VOLATILE,
                          KEY_ALL_ACCESS, NULL, phkResult, NULL);
}

/* RegCreateKeyW */
LONG RegCreateKeyW(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult) {
    return RegCreateKeyExW(hKey, lpSubKey, 0, NULL, REG_OPTION_NON_VOLATILE,
                          KEY_ALL_ACCESS, NULL, phkResult, NULL);
}

/* RegCloseKey */
LONG RegCloseKey(HKEY hKey) {
    if (!hKey) {
        return ERROR_INVALID_HANDLE;
    }

    /* Don't close predefined keys */
    if (is_predefined_key(hKey)) {
        return ERROR_SUCCESS;
    }

    free_registry_handle(hKey);
    return ERROR_SUCCESS;
}

/* RegQueryValueExA */
LONG RegQueryValueExA(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved,
                     LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) {
    if (!lpcbData) {
        return ERROR_INVALID_PARAMETER;
    }

    LONG result = ensure_initialized();
    if (result != ERROR_SUCCESS) {
        return result;
    }

    sqlite3_int64 key_id;

    if (is_predefined_key(hKey)) {
        const char* root_name = get_predefined_key_name(hKey);
        result = db_open_key(root_name, &key_id);
        if (result != ERROR_SUCCESS) {
            return result;
        }
    } else {
        PREGISTRY_KEY reg_key = get_registry_key(hKey);
        if (!reg_key) {
            return ERROR_INVALID_HANDLE;
        }
        key_id = reg_key->key_id;
    }

    return db_query_value(key_id, lpValueName, lpType, lpData, lpcbData);
}

/* RegQueryValueExW */
LONG RegQueryValueExW(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved,
                     LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) {
    char* value_name_utf8 = wchar_to_utf8(lpValueName);
    LONG result = RegQueryValueExA(hKey, value_name_utf8, lpReserved, lpType, lpData, lpcbData);
    free(value_name_utf8);
    return result;
}

/* RegSetValueExA */
LONG RegSetValueExA(HKEY hKey, LPCSTR lpValueName, DWORD Reserved,
                   DWORD dwType, const BYTE* lpData, DWORD cbData) {
    LONG result = ensure_initialized();
    if (result != ERROR_SUCCESS) {
        return result;
    }

    sqlite3_int64 key_id;

    if (is_predefined_key(hKey)) {
        const char* root_name = get_predefined_key_name(hKey);
        result = db_open_key(root_name, &key_id);
        if (result != ERROR_SUCCESS) {
            return result;
        }
    } else {
        PREGISTRY_KEY reg_key = get_registry_key(hKey);
        if (!reg_key) {
            return ERROR_INVALID_HANDLE;
        }
        key_id = reg_key->key_id;
    }

    return db_set_value(key_id, lpValueName, dwType, lpData, cbData);
}

/* RegSetValueExW */
LONG RegSetValueExW(HKEY hKey, LPCWSTR lpValueName, DWORD Reserved,
                   DWORD dwType, const BYTE* lpData, DWORD cbData) {
    char* value_name_utf8 = wchar_to_utf8(lpValueName);
    LONG result = RegSetValueExA(hKey, value_name_utf8, Reserved, dwType, lpData, cbData);
    free(value_name_utf8);
    return result;
}

/* RegDeleteKeyA */
LONG RegDeleteKeyA(HKEY hKey, LPCSTR lpSubKey) {
    LONG result = ensure_initialized();
    if (result != ERROR_SUCCESS) {
        return result;
    }

    char* full_path = build_full_path(hKey, lpSubKey);
    if (!full_path) {
        return ERROR_INVALID_HANDLE;
    }

    result = db_delete_key(full_path);
    free(full_path);
    return result;
}

/* RegDeleteKeyW */
LONG RegDeleteKeyW(HKEY hKey, LPCWSTR lpSubKey) {
    char* sub_key_utf8 = wchar_to_utf8(lpSubKey);
    LONG result = RegDeleteKeyA(hKey, sub_key_utf8);
    free(sub_key_utf8);
    return result;
}

/* RegDeleteValueA */
LONG RegDeleteValueA(HKEY hKey, LPCSTR lpValueName) {
    LONG result = ensure_initialized();
    if (result != ERROR_SUCCESS) {
        return result;
    }

    sqlite3_int64 key_id;

    if (is_predefined_key(hKey)) {
        const char* root_name = get_predefined_key_name(hKey);
        result = db_open_key(root_name, &key_id);
        if (result != ERROR_SUCCESS) {
            return result;
        }
    } else {
        PREGISTRY_KEY reg_key = get_registry_key(hKey);
        if (!reg_key) {
            return ERROR_INVALID_HANDLE;
        }
        key_id = reg_key->key_id;
    }

    return db_delete_value(key_id, lpValueName);
}

/* RegDeleteValueW */
LONG RegDeleteValueW(HKEY hKey, LPCWSTR lpValueName) {
    char* value_name_utf8 = wchar_to_utf8(lpValueName);
    LONG result = RegDeleteValueA(hKey, value_name_utf8);
    free(value_name_utf8);
    return result;
}

/* RegEnumKeyExA */
LONG RegEnumKeyExA(HKEY hKey, DWORD dwIndex, LPSTR lpName, LPDWORD lpcchName,
                  LPDWORD lpReserved, LPSTR lpClass, LPDWORD lpcchClass,
                  PFILETIME lpftLastWriteTime) {
    if (!lpName || !lpcchName) {
        return ERROR_INVALID_PARAMETER;
    }

    LONG result = ensure_initialized();
    if (result != ERROR_SUCCESS) {
        return result;
    }

    sqlite3_int64 key_id;

    if (is_predefined_key(hKey)) {
        const char* root_name = get_predefined_key_name(hKey);
        result = db_open_key(root_name, &key_id);
        if (result != ERROR_SUCCESS) {
            return result;
        }
    } else {
        PREGISTRY_KEY reg_key = get_registry_key(hKey);
        if (!reg_key) {
            return ERROR_INVALID_HANDLE;
        }
        key_id = reg_key->key_id;
    }

    return db_enumerate_keys(key_id, dwIndex, lpName, lpcchName, lpClass, lpcchClass, lpftLastWriteTime);
}

/* RegEnumKeyExW */
LONG RegEnumKeyExW(HKEY hKey, DWORD dwIndex, LPWSTR lpName, LPDWORD lpcchName,
                  LPDWORD lpReserved, LPWSTR lpClass, LPDWORD lpcchClass,
                  PFILETIME lpftLastWriteTime) {
    if (!lpName || !lpcchName) {
        return ERROR_INVALID_PARAMETER;
    }

    char* name_buffer = malloc(*lpcchName);
    char* class_buffer = (lpClass && lpcchClass) ? malloc(*lpcchClass) : NULL;

    DWORD name_len = *lpcchName;
    DWORD class_len = lpcchClass ? *lpcchClass : 0;

    LONG result = RegEnumKeyExA(hKey, dwIndex, name_buffer, &name_len, lpReserved,
                               class_buffer, &class_len, lpftLastWriteTime);

    if (result == ERROR_SUCCESS || result == ERROR_MORE_DATA) {
        if (result == ERROR_SUCCESS) {
            wchar_t* wname = utf8_to_wchar(name_buffer);
            if (wname) {
                wcsncpy(lpName, wname, *lpcchName);
                free(wname);
            }

            if (class_buffer && lpClass) {
                wchar_t* wclass = utf8_to_wchar(class_buffer);
                if (wclass) {
                    wcsncpy(lpClass, wclass, *lpcchClass);
                    free(wclass);
                }
            }
        }

        *lpcchName = name_len;
        if (lpcchClass) {
            *lpcchClass = class_len;
        }
    }

    free(name_buffer);
    free(class_buffer);
    return result;
}

/* RegEnumValueA */
LONG RegEnumValueA(HKEY hKey, DWORD dwIndex, LPSTR lpValueName, LPDWORD lpcchValueName,
                  LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) {
    if (!lpValueName || !lpcchValueName) {
        return ERROR_INVALID_PARAMETER;
    }

    LONG result = ensure_initialized();
    if (result != ERROR_SUCCESS) {
        return result;
    }

    sqlite3_int64 key_id;

    if (is_predefined_key(hKey)) {
        const char* root_name = get_predefined_key_name(hKey);
        result = db_open_key(root_name, &key_id);
        if (result != ERROR_SUCCESS) {
            return result;
        }
    } else {
        PREGISTRY_KEY reg_key = get_registry_key(hKey);
        if (!reg_key) {
            return ERROR_INVALID_HANDLE;
        }
        key_id = reg_key->key_id;
    }

    return db_enumerate_values(key_id, dwIndex, lpValueName, lpcchValueName, lpType, lpData, lpcbData);
}

/* RegEnumValueW */
LONG RegEnumValueW(HKEY hKey, DWORD dwIndex, LPWSTR lpValueName, LPDWORD lpcchValueName,
                  LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) {
    if (!lpValueName || !lpcchValueName) {
        return ERROR_INVALID_PARAMETER;
    }

    char* name_buffer = malloc(*lpcchValueName);
    DWORD name_len = *lpcchValueName;

    LONG result = RegEnumValueA(hKey, dwIndex, name_buffer, &name_len, lpReserved, lpType, lpData, lpcbData);

    if (result == ERROR_SUCCESS || result == ERROR_MORE_DATA) {
        if (result == ERROR_SUCCESS) {
            wchar_t* wname = utf8_to_wchar(name_buffer);
            if (wname) {
                wcsncpy(lpValueName, wname, *lpcchValueName);
                free(wname);
            }
        }
        *lpcchValueName = name_len;
    }

    free(name_buffer);
    return result;
}

/* RegQueryInfoKeyA */
LONG RegQueryInfoKeyA(HKEY hKey, LPSTR lpClass, LPDWORD lpcchClass, LPDWORD lpReserved,
                     LPDWORD lpcSubKeys, LPDWORD lpcbMaxSubKeyLen, LPDWORD lpcbMaxClassLen,
                     LPDWORD lpcValues, LPDWORD lpcbMaxValueNameLen, LPDWORD lpcbMaxValueLen,
                     LPDWORD lpcbSecurityDescriptor, PFILETIME lpftLastWriteTime) {
    LONG result = ensure_initialized();
    if (result != ERROR_SUCCESS) {
        return result;
    }

    sqlite3_int64 key_id;

    if (is_predefined_key(hKey)) {
        const char* root_name = get_predefined_key_name(hKey);
        result = db_open_key(root_name, &key_id);
        if (result != ERROR_SUCCESS) {
            return result;
        }
    } else {
        PREGISTRY_KEY reg_key = get_registry_key(hKey);
        if (!reg_key) {
            return ERROR_INVALID_HANDLE;
        }
        key_id = reg_key->key_id;
    }

    return db_query_key_info(key_id, lpcSubKeys, lpcbMaxSubKeyLen, lpcbMaxClassLen,
                            lpcValues, lpcbMaxValueNameLen, lpcbMaxValueLen, lpftLastWriteTime);
}

/* RegQueryInfoKeyW */
LONG RegQueryInfoKeyW(HKEY hKey, LPWSTR lpClass, LPDWORD lpcchClass, LPDWORD lpReserved,
                     LPDWORD lpcSubKeys, LPDWORD lpcbMaxSubKeyLen, LPDWORD lpcbMaxClassLen,
                     LPDWORD lpcValues, LPDWORD lpcbMaxValueNameLen, LPDWORD lpcbMaxValueLen,
                     LPDWORD lpcbSecurityDescriptor, PFILETIME lpftLastWriteTime) {
    return RegQueryInfoKeyA(hKey, (LPSTR)lpClass, lpcchClass, lpReserved, lpcSubKeys,
                           lpcbMaxSubKeyLen, lpcbMaxClassLen, lpcValues, lpcbMaxValueNameLen,
                           lpcbMaxValueLen, lpcbSecurityDescriptor, lpftLastWriteTime);
}

/* RegDeleteTreeA */
LONG RegDeleteTreeA(HKEY hKey, LPCSTR lpSubKey) {
    LONG result = ensure_initialized();
    if (result != ERROR_SUCCESS) {
        return result;
    }

    char* full_path = build_full_path(hKey, lpSubKey);
    if (!full_path) {
        return ERROR_INVALID_HANDLE;
    }

    result = db_delete_key_tree(full_path);
    free(full_path);
    return result;
}

/* RegDeleteTreeW */
LONG RegDeleteTreeW(HKEY hKey, LPCWSTR lpSubKey) {
    char* sub_key_utf8 = wchar_to_utf8(lpSubKey);
    LONG result = RegDeleteTreeA(hKey, sub_key_utf8);
    free(sub_key_utf8);
    return result;
}

/* RegFlushKey */
LONG RegFlushKey(HKEY hKey) {
    /* SQLite handles this automatically with transactions */
    return ERROR_SUCCESS;
}
