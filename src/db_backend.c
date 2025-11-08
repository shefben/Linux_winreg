#include "registry_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>

static sqlite3* g_db = NULL;
static pthread_mutex_t g_db_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Database schema */
static const char* SCHEMA_SQL =
    "CREATE TABLE IF NOT EXISTS registry_keys ("
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "    path TEXT NOT NULL UNIQUE,"
    "    class TEXT,"
    "    options INTEGER DEFAULT 0,"
    "    created_time INTEGER DEFAULT (strftime('%s', 'now')),"
    "    modified_time INTEGER DEFAULT (strftime('%s', 'now'))"
    ");"
    ""
    "CREATE TABLE IF NOT EXISTS registry_values ("
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "    key_id INTEGER NOT NULL,"
    "    name TEXT NOT NULL,"
    "    type INTEGER NOT NULL,"
    "    data BLOB,"
    "    data_size INTEGER,"
    "    FOREIGN KEY(key_id) REFERENCES registry_keys(id) ON DELETE CASCADE,"
    "    UNIQUE(key_id, name)"
    ");"
    ""
    "CREATE INDEX IF NOT EXISTS idx_keys_path ON registry_keys(path);"
    "CREATE INDEX IF NOT EXISTS idx_values_key ON registry_values(key_id);"
    "";

/* Get database file path */
static char* get_db_path(void) {
    const char* home = getenv("HOME");
    if (!home) {
        home = "/tmp";
    }

    char* db_dir = malloc(strlen(home) + 32);
    sprintf(db_dir, "%s/.winreg", home);

    /* Create directory if it doesn't exist */
    mkdir(db_dir, 0755);

    char* db_path = malloc(strlen(db_dir) + 32);
    sprintf(db_path, "%s/registry.db", db_dir);

    free(db_dir);
    return db_path;
}

/* Initialize database */
LONG db_init(void) {
    if (g_db != NULL) {
        return ERROR_SUCCESS;
    }

    char* db_path = get_db_path();
    int rc = sqlite3_open(db_path, &g_db);
    free(db_path);

    if (rc != SQLITE_OK) {
        if (g_db) {
            sqlite3_close(g_db);
            g_db = NULL;
        }
        return ERROR_CANTOPEN;
    }

    /* Enable foreign keys */
    sqlite3_exec(g_db, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);

    /* Create schema */
    char* err_msg = NULL;
    rc = sqlite3_exec(g_db, SCHEMA_SQL, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        sqlite3_free(err_msg);
        sqlite3_close(g_db);
        g_db = NULL;
        return ERROR_BADDB;
    }

    /* Create predefined root keys */
    const char* root_keys[] = {
        "HKEY_CLASSES_ROOT",
        "HKEY_CURRENT_USER",
        "HKEY_LOCAL_MACHINE",
        "HKEY_USERS",
        "HKEY_CURRENT_CONFIG"
    };

    for (int i = 0; i < 5; i++) {
        sqlite3_int64 dummy_id;
        DWORD dummy_disp;
        db_create_key(root_keys[i], REG_OPTION_NON_VOLATILE, &dummy_id, &dummy_disp);
    }

    return ERROR_SUCCESS;
}

/* Cleanup database */
void db_cleanup(void) {
    if (g_db) {
        sqlite3_close(g_db);
        g_db = NULL;
    }
}

/* Get database connection */
sqlite3* db_get_connection(void) {
    return g_db;
}

/* Create or open a registry key */
LONG db_create_key(const char* path, DWORD options, sqlite3_int64* key_id, DWORD* disposition) {
    if (!g_db || !path || !key_id) {
        return ERROR_INVALID_PARAMETER;
    }

    pthread_mutex_lock(&g_db_mutex);

    /* Check if key already exists */
    sqlite3_stmt* stmt;
    const char* sql = "SELECT id FROM registry_keys WHERE path = ?;";

    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_db_mutex);
        return ERROR_BADDB;
    }

    sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        /* Key exists */
        *key_id = sqlite3_column_int64(stmt, 0);
        if (disposition) {
            *disposition = REG_OPENED_EXISTING_KEY;
        }
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&g_db_mutex);
        return ERROR_SUCCESS;
    }

    sqlite3_finalize(stmt);

    /* Create new key */
    sql = "INSERT INTO registry_keys (path, options) VALUES (?, ?);";
    rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_db_mutex);
        return ERROR_BADDB;
    }

    sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, options);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        pthread_mutex_unlock(&g_db_mutex);
        return ERROR_CANTWRITE;
    }

    *key_id = sqlite3_last_insert_rowid(g_db);
    if (disposition) {
        *disposition = REG_CREATED_NEW_KEY;
    }

    pthread_mutex_unlock(&g_db_mutex);
    return ERROR_SUCCESS;
}

/* Open a registry key */
LONG db_open_key(const char* path, sqlite3_int64* key_id) {
    if (!g_db || !path || !key_id) {
        return ERROR_INVALID_PARAMETER;
    }

    pthread_mutex_lock(&g_db_mutex);

    sqlite3_stmt* stmt;
    const char* sql = "SELECT id FROM registry_keys WHERE path = ?;";

    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_db_mutex);
        return ERROR_BADDB;
    }

    sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);

    if (rc == SQLITE_ROW) {
        *key_id = sqlite3_column_int64(stmt, 0);
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&g_db_mutex);
        return ERROR_SUCCESS;
    }

    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&g_db_mutex);
    return ERROR_FILE_NOT_FOUND;
}

/* Delete a registry key */
LONG db_delete_key(const char* path) {
    if (!g_db || !path) {
        return ERROR_INVALID_PARAMETER;
    }

    pthread_mutex_lock(&g_db_mutex);

    /* Check if key has subkeys */
    sqlite3_stmt* stmt;
    char pattern[1024];
    snprintf(pattern, sizeof(pattern), "%s\\%%", path);

    const char* sql = "SELECT COUNT(*) FROM registry_keys WHERE path LIKE ? ESCAPE '\\';";

    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_db_mutex);
        return ERROR_BADDB;
    }

    sqlite3_bind_text(stmt, 1, pattern, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);

    int count = 0;
    if (rc == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);

    if (count > 0) {
        pthread_mutex_unlock(&g_db_mutex);
        return ERROR_KEY_HAS_CHILDREN;
    }

    /* Delete the key */
    sql = "DELETE FROM registry_keys WHERE path = ?;";
    rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_db_mutex);
        return ERROR_BADDB;
    }

    sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        pthread_mutex_unlock(&g_db_mutex);
        return ERROR_CANTWRITE;
    }

    if (sqlite3_changes(g_db) == 0) {
        pthread_mutex_unlock(&g_db_mutex);
        return ERROR_FILE_NOT_FOUND;
    }

    pthread_mutex_unlock(&g_db_mutex);
    return ERROR_SUCCESS;
}

/* Delete a registry key tree */
LONG db_delete_key_tree(const char* path) {
    if (!g_db || !path) {
        return ERROR_INVALID_PARAMETER;
    }

    pthread_mutex_lock(&g_db_mutex);

    /* Delete all subkeys and the key itself */
    sqlite3_stmt* stmt;
    char pattern[1024];
    snprintf(pattern, sizeof(pattern), "%s%%", path);

    const char* sql = "DELETE FROM registry_keys WHERE path = ? OR path LIKE ?;";

    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_db_mutex);
        return ERROR_BADDB;
    }

    sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, pattern, -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        pthread_mutex_unlock(&g_db_mutex);
        return ERROR_CANTWRITE;
    }

    pthread_mutex_unlock(&g_db_mutex);
    return ERROR_SUCCESS;
}

/* Check if a key exists */
LONG db_key_exists(const char* path, BOOL* exists) {
    if (!g_db || !path || !exists) {
        return ERROR_INVALID_PARAMETER;
    }

    pthread_mutex_lock(&g_db_mutex);

    sqlite3_stmt* stmt;
    const char* sql = "SELECT 1 FROM registry_keys WHERE path = ? LIMIT 1;";

    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_db_mutex);
        return ERROR_BADDB;
    }

    sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);

    *exists = (rc == SQLITE_ROW) ? TRUE : FALSE;

    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&g_db_mutex);
    return ERROR_SUCCESS;
}

/* Enumerate registry keys */
LONG db_enumerate_keys(sqlite3_int64 parent_id, DWORD index, char* name, DWORD* name_len,
                       char* class_name, DWORD* class_len, FILETIME* last_write) {
    if (!g_db || !name || !name_len) {
        return ERROR_INVALID_PARAMETER;
    }

    pthread_mutex_lock(&g_db_mutex);

    /* Get parent path */
    sqlite3_stmt* stmt;
    const char* sql = "SELECT path FROM registry_keys WHERE id = ?;";

    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_db_mutex);
        return ERROR_BADDB;
    }

    sqlite3_bind_int64(stmt, 1, parent_id);
    rc = sqlite3_step(stmt);

    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&g_db_mutex);
        return ERROR_INVALID_HANDLE;
    }

    const char* parent_path = (const char*)sqlite3_column_text(stmt, 0);
    char pattern[1024];
    snprintf(pattern, sizeof(pattern), "%s\\%%", parent_path);

    sqlite3_finalize(stmt);

    /* Get subkeys */
    sql = "SELECT path, class, modified_time FROM registry_keys "
          "WHERE path LIKE ? ESCAPE '\\' "
          "AND path NOT LIKE ? ESCAPE '\\' "
          "ORDER BY path LIMIT 1 OFFSET ?;";

    rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_db_mutex);
        return ERROR_BADDB;
    }

    char pattern2[1024];
    snprintf(pattern2, sizeof(pattern2), "%s\\%%\\%%", parent_path);

    sqlite3_bind_text(stmt, 1, pattern, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, pattern2, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, index);

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&g_db_mutex);
        return ERROR_NO_MORE_ITEMS;
    }

    const char* full_path = (const char*)sqlite3_column_text(stmt, 0);
    const char* last_backslash = strrchr(full_path, '\\');
    const char* key_name = last_backslash ? last_backslash + 1 : full_path;

    size_t len = strlen(key_name);
    if (len >= *name_len) {
        *name_len = len + 1;
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&g_db_mutex);
        return ERROR_MORE_DATA;
    }

    strcpy(name, key_name);
    *name_len = len;

    if (class_name && class_len) {
        const char* cls = (const char*)sqlite3_column_text(stmt, 1);
        if (cls) {
            len = strlen(cls);
            if (len >= *class_len) {
                *class_len = len + 1;
                sqlite3_finalize(stmt);
                pthread_mutex_unlock(&g_db_mutex);
                return ERROR_MORE_DATA;
            }
            strcpy(class_name, cls);
            *class_len = len;
        } else {
            if (*class_len > 0) {
                class_name[0] = '\0';
            }
            *class_len = 0;
        }
    }

    if (last_write) {
        sqlite3_int64 mtime = sqlite3_column_int64(stmt, 2);
        /* Convert Unix timestamp to FILETIME */
        ULONGLONG ll = (mtime + 11644473600LL) * 10000000LL;
        last_write->dwLowDateTime = (DWORD)ll;
        last_write->dwHighDateTime = (DWORD)(ll >> 32);
    }

    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&g_db_mutex);
    return ERROR_SUCCESS;
}

/* Set a registry value */
LONG db_set_value(sqlite3_int64 key_id, const char* value_name, DWORD type,
                  const BYTE* data, DWORD data_size) {
    if (!g_db) {
        return ERROR_INVALID_PARAMETER;
    }

    pthread_mutex_lock(&g_db_mutex);

    /* Insert or replace value */
    sqlite3_stmt* stmt;
    const char* sql = "INSERT OR REPLACE INTO registry_values "
                     "(key_id, name, type, data, data_size) VALUES (?, ?, ?, ?, ?);";

    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_db_mutex);
        return ERROR_BADDB;
    }

    sqlite3_bind_int64(stmt, 1, key_id);
    sqlite3_bind_text(stmt, 2, value_name ? value_name : "", -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, type);
    sqlite3_bind_blob(stmt, 4, data, data_size, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 5, data_size);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        pthread_mutex_unlock(&g_db_mutex);
        return ERROR_CANTWRITE;
    }

    /* Update key modified time */
    sql = "UPDATE registry_keys SET modified_time = strftime('%s', 'now') WHERE id = ?;";
    rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, key_id);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    pthread_mutex_unlock(&g_db_mutex);
    return ERROR_SUCCESS;
}

/* Query a registry value */
LONG db_query_value(sqlite3_int64 key_id, const char* value_name, DWORD* type,
                    BYTE* data, DWORD* data_size) {
    if (!g_db || !data_size) {
        return ERROR_INVALID_PARAMETER;
    }

    pthread_mutex_lock(&g_db_mutex);

    sqlite3_stmt* stmt;
    const char* sql = "SELECT type, data, data_size FROM registry_values "
                     "WHERE key_id = ? AND name = ?;";

    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_db_mutex);
        return ERROR_BADDB;
    }

    sqlite3_bind_int64(stmt, 1, key_id);
    sqlite3_bind_text(stmt, 2, value_name ? value_name : "", -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&g_db_mutex);
        return ERROR_FILE_NOT_FOUND;
    }

    if (type) {
        *type = sqlite3_column_int(stmt, 0);
    }

    int stored_size = sqlite3_column_int(stmt, 2);

    if (data == NULL || *data_size < (DWORD)stored_size) {
        *data_size = stored_size;
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&g_db_mutex);
        return ERROR_MORE_DATA;
    }

    const void* blob = sqlite3_column_blob(stmt, 1);
    if (blob && stored_size > 0) {
        memcpy(data, blob, stored_size);
    }
    *data_size = stored_size;

    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&g_db_mutex);
    return ERROR_SUCCESS;
}

/* Delete a registry value */
LONG db_delete_value(sqlite3_int64 key_id, const char* value_name) {
    if (!g_db) {
        return ERROR_INVALID_PARAMETER;
    }

    pthread_mutex_lock(&g_db_mutex);

    sqlite3_stmt* stmt;
    const char* sql = "DELETE FROM registry_values WHERE key_id = ? AND name = ?;";

    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_db_mutex);
        return ERROR_BADDB;
    }

    sqlite3_bind_int64(stmt, 1, key_id);
    sqlite3_bind_text(stmt, 2, value_name ? value_name : "", -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        pthread_mutex_unlock(&g_db_mutex);
        return ERROR_CANTWRITE;
    }

    if (sqlite3_changes(g_db) == 0) {
        pthread_mutex_unlock(&g_db_mutex);
        return ERROR_FILE_NOT_FOUND;
    }

    pthread_mutex_unlock(&g_db_mutex);
    return ERROR_SUCCESS;
}

/* Enumerate registry values */
LONG db_enumerate_values(sqlite3_int64 key_id, DWORD index, char* value_name,
                         DWORD* value_name_len, DWORD* type, BYTE* data, DWORD* data_size) {
    if (!g_db || !value_name || !value_name_len) {
        return ERROR_INVALID_PARAMETER;
    }

    pthread_mutex_lock(&g_db_mutex);

    sqlite3_stmt* stmt;
    const char* sql = "SELECT name, type, data, data_size FROM registry_values "
                     "WHERE key_id = ? ORDER BY name LIMIT 1 OFFSET ?;";

    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_db_mutex);
        return ERROR_BADDB;
    }

    sqlite3_bind_int64(stmt, 1, key_id);
    sqlite3_bind_int(stmt, 2, index);

    rc = sqlite3_step(stmt);

    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&g_db_mutex);
        return ERROR_NO_MORE_ITEMS;
    }

    const char* name = (const char*)sqlite3_column_text(stmt, 0);
    size_t len = strlen(name);

    if (len >= *value_name_len) {
        *value_name_len = len + 1;
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&g_db_mutex);
        return ERROR_MORE_DATA;
    }

    strcpy(value_name, name);
    *value_name_len = len;

    if (type) {
        *type = sqlite3_column_int(stmt, 1);
    }

    if (data && data_size) {
        int stored_size = sqlite3_column_int(stmt, 3);

        if (*data_size < (DWORD)stored_size) {
            *data_size = stored_size;
            sqlite3_finalize(stmt);
            pthread_mutex_unlock(&g_db_mutex);
            return ERROR_MORE_DATA;
        }

        const void* blob = sqlite3_column_blob(stmt, 2);
        if (blob && stored_size > 0) {
            memcpy(data, blob, stored_size);
        }
        *data_size = stored_size;
    } else if (data_size) {
        *data_size = sqlite3_column_int(stmt, 3);
    }

    sqlite3_finalize(stmt);
    pthread_mutex_unlock(&g_db_mutex);
    return ERROR_SUCCESS;
}

/* Query key information */
LONG db_query_key_info(sqlite3_int64 key_id, DWORD* subkey_count, DWORD* max_subkey_len,
                       DWORD* max_class_len, DWORD* value_count, DWORD* max_value_name_len,
                       DWORD* max_value_len, FILETIME* last_write) {
    if (!g_db) {
        return ERROR_INVALID_PARAMETER;
    }

    pthread_mutex_lock(&g_db_mutex);

    /* Get parent path and modified time */
    sqlite3_stmt* stmt;
    const char* sql = "SELECT path, modified_time FROM registry_keys WHERE id = ?;";

    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        pthread_mutex_unlock(&g_db_mutex);
        return ERROR_BADDB;
    }

    sqlite3_bind_int64(stmt, 1, key_id);
    rc = sqlite3_step(stmt);

    if (rc != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        pthread_mutex_unlock(&g_db_mutex);
        return ERROR_INVALID_HANDLE;
    }

    const char* parent_path = (const char*)sqlite3_column_text(stmt, 0);
    sqlite3_int64 mtime = sqlite3_column_int64(stmt, 1);

    char pattern[1024];
    snprintf(pattern, sizeof(pattern), "%s\\%%", parent_path);
    char pattern2[1024];
    snprintf(pattern2, sizeof(pattern2), "%s\\%%\\%%", parent_path);

    sqlite3_finalize(stmt);

    /* Count subkeys and get max subkey name length */
    if (subkey_count || max_subkey_len) {
        sql = "SELECT COUNT(*), MAX(LENGTH(SUBSTR(path, LENGTH(?) + 2))) "
              "FROM registry_keys "
              "WHERE path LIKE ? ESCAPE '\\' "
              "AND path NOT LIKE ? ESCAPE '\\';";

        rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
        if (rc == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, parent_path, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, pattern, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, pattern2, -1, SQLITE_STATIC);

            if (sqlite3_step(stmt) == SQLITE_ROW) {
                if (subkey_count) {
                    *subkey_count = sqlite3_column_int(stmt, 0);
                }
                if (max_subkey_len) {
                    *max_subkey_len = sqlite3_column_int(stmt, 1);
                }
            }
            sqlite3_finalize(stmt);
        }
    }

    /* Get max class length */
    if (max_class_len) {
        sql = "SELECT MAX(LENGTH(class)) FROM registry_keys "
              "WHERE path LIKE ? ESCAPE '\\' "
              "AND path NOT LIKE ? ESCAPE '\\';";

        rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
        if (rc == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, pattern, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, pattern2, -1, SQLITE_STATIC);

            if (sqlite3_step(stmt) == SQLITE_ROW) {
                *max_class_len = sqlite3_column_int(stmt, 0);
            }
            sqlite3_finalize(stmt);
        }
    }

    /* Count values and get max value name/data length */
    if (value_count || max_value_name_len || max_value_len) {
        sql = "SELECT COUNT(*), MAX(LENGTH(name)), MAX(data_size) "
              "FROM registry_values WHERE key_id = ?;";

        rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
        if (rc == SQLITE_OK) {
            sqlite3_bind_int64(stmt, 1, key_id);

            if (sqlite3_step(stmt) == SQLITE_ROW) {
                if (value_count) {
                    *value_count = sqlite3_column_int(stmt, 0);
                }
                if (max_value_name_len) {
                    *max_value_name_len = sqlite3_column_int(stmt, 1);
                }
                if (max_value_len) {
                    *max_value_len = sqlite3_column_int(stmt, 2);
                }
            }
            sqlite3_finalize(stmt);
        }
    }

    /* Set last write time */
    if (last_write) {
        ULONGLONG ll = (mtime + 11644473600LL) * 10000000LL;
        last_write->dwLowDateTime = (DWORD)ll;
        last_write->dwHighDateTime = (DWORD)(ll >> 32);
    }

    pthread_mutex_unlock(&g_db_mutex);
    return ERROR_SUCCESS;
}
