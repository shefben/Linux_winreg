#ifndef _REGISTRY_INTERNAL_H
#define _REGISTRY_INTERNAL_H

#include <sqlite3.h>
#include <pthread.h>
#include "../include/winreg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Internal registry key structure */
typedef struct _REGISTRY_KEY {
    DWORD magic;            /* Magic number for validation */
    sqlite3_int64 key_id;   /* SQLite key ID */
    DWORD access_rights;    /* Access rights */
    char* path;             /* Full key path */
    BOOL is_predefined;     /* Is this a predefined key */
} REGISTRY_KEY, *PREGISTRY_KEY;

#define REGISTRY_KEY_MAGIC 0x52454758  /* "REGX" */

/* Database operations */
LONG db_init(void);
void db_cleanup(void);
sqlite3* db_get_connection(void);

/* Key operations */
LONG db_create_key(const char* path, DWORD options, sqlite3_int64* key_id, DWORD* disposition);
LONG db_open_key(const char* path, sqlite3_int64* key_id);
LONG db_delete_key(const char* path);
LONG db_delete_key_tree(const char* path);
LONG db_key_exists(const char* path, BOOL* exists);
LONG db_enumerate_keys(sqlite3_int64 parent_id, DWORD index, char* name, DWORD* name_len,
                       char* class_name, DWORD* class_len, FILETIME* last_write);

/* Value operations */
LONG db_set_value(sqlite3_int64 key_id, const char* value_name, DWORD type,
                  const BYTE* data, DWORD data_size);
LONG db_query_value(sqlite3_int64 key_id, const char* value_name, DWORD* type,
                    BYTE* data, DWORD* data_size);
LONG db_delete_value(sqlite3_int64 key_id, const char* value_name);
LONG db_enumerate_values(sqlite3_int64 key_id, DWORD index, char* value_name,
                         DWORD* value_name_len, DWORD* type, BYTE* data, DWORD* data_size);
LONG db_query_key_info(sqlite3_int64 key_id, DWORD* subkey_count, DWORD* max_subkey_len,
                       DWORD* max_class_len, DWORD* value_count, DWORD* max_value_name_len,
                       DWORD* max_value_len, FILETIME* last_write);

/* Path utilities */
char* build_full_path(HKEY hKey, const char* sub_key);
const char* get_predefined_key_name(HKEY hKey);
BOOL is_predefined_key(HKEY hKey);

/* Handle management */
HKEY create_registry_handle(sqlite3_int64 key_id, DWORD access_rights, const char* path, BOOL is_predefined);
PREGISTRY_KEY get_registry_key(HKEY hKey);
void free_registry_handle(HKEY hKey);

/* Wide char conversion utilities */
char* wchar_to_utf8(const wchar_t* wstr);
wchar_t* utf8_to_wchar(const char* str);

/* Thread safety */
void registry_lock(void);
void registry_unlock(void);

#ifdef __cplusplus
}
#endif

#endif /* _REGISTRY_INTERNAL_H */
