#include "registry_internal.h"
#include <stdlib.h>
#include <string.h>
#include <iconv.h>

static pthread_mutex_t g_registry_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Thread safety */
void registry_lock(void) {
    pthread_mutex_lock(&g_registry_mutex);
}

void registry_unlock(void) {
    pthread_mutex_unlock(&g_registry_mutex);
}

/* Get predefined key name */
const char* get_predefined_key_name(HKEY hKey) {
    switch ((ULONG_PTR)hKey) {
        case (ULONG_PTR)HKEY_CLASSES_ROOT:
            return "HKEY_CLASSES_ROOT";
        case (ULONG_PTR)HKEY_CURRENT_USER:
            return "HKEY_CURRENT_USER";
        case (ULONG_PTR)HKEY_LOCAL_MACHINE:
            return "HKEY_LOCAL_MACHINE";
        case (ULONG_PTR)HKEY_USERS:
            return "HKEY_USERS";
        case (ULONG_PTR)HKEY_CURRENT_CONFIG:
            return "HKEY_CURRENT_CONFIG";
        default:
            return NULL;
    }
}

/* Check if key is predefined */
BOOL is_predefined_key(HKEY hKey) {
    return get_predefined_key_name(hKey) != NULL;
}

/* Build full path from hKey and subkey */
char* build_full_path(HKEY hKey, const char* sub_key) {
    const char* root_name = NULL;

    if (is_predefined_key(hKey)) {
        root_name = get_predefined_key_name(hKey);
    } else {
        PREGISTRY_KEY reg_key = get_registry_key(hKey);
        if (!reg_key) {
            return NULL;
        }
        root_name = reg_key->path;
    }

    if (!root_name) {
        return NULL;
    }

    size_t total_len = strlen(root_name);

    if (sub_key && sub_key[0] != '\0') {
        total_len += strlen(sub_key) + 1;  /* +1 for backslash */
    }

    char* full_path = malloc(total_len + 1);
    if (!full_path) {
        return NULL;
    }

    strcpy(full_path, root_name);

    if (sub_key && sub_key[0] != '\0') {
        strcat(full_path, "\\");
        strcat(full_path, sub_key);
    }

    return full_path;
}

/* Create a registry handle */
HKEY create_registry_handle(sqlite3_int64 key_id, DWORD access_rights, const char* path, BOOL is_predefined) {
    PREGISTRY_KEY reg_key = malloc(sizeof(REGISTRY_KEY));
    if (!reg_key) {
        return NULL;
    }

    reg_key->magic = REGISTRY_KEY_MAGIC;
    reg_key->key_id = key_id;
    reg_key->access_rights = access_rights;
    reg_key->is_predefined = is_predefined;

    if (path) {
        reg_key->path = strdup(path);
    } else {
        reg_key->path = NULL;
    }

    return (HKEY)reg_key;
}

/* Get registry key structure from handle */
PREGISTRY_KEY get_registry_key(HKEY hKey) {
    if (!hKey) {
        return NULL;
    }

    /* Check if it's a predefined key */
    if (is_predefined_key(hKey)) {
        return NULL;  /* Predefined keys are handled specially */
    }

    PREGISTRY_KEY reg_key = (PREGISTRY_KEY)hKey;

    /* Validate magic number */
    if (reg_key->magic != REGISTRY_KEY_MAGIC) {
        return NULL;
    }

    return reg_key;
}

/* Free a registry handle */
void free_registry_handle(HKEY hKey) {
    if (!hKey || is_predefined_key(hKey)) {
        return;
    }

    PREGISTRY_KEY reg_key = get_registry_key(hKey);
    if (reg_key) {
        if (reg_key->path) {
            free(reg_key->path);
        }
        reg_key->magic = 0;  /* Invalidate */
        free(reg_key);
    }
}

/* Convert wide char to UTF-8 */
char* wchar_to_utf8(const wchar_t* wstr) {
    if (!wstr) {
        return NULL;
    }

    size_t wlen = wcslen(wstr);
    size_t max_len = wlen * 4 + 1;  /* UTF-8 max 4 bytes per char */

    char* result = malloc(max_len);
    if (!result) {
        return NULL;
    }

    iconv_t cd = iconv_open("UTF-8", "WCHAR_T");
    if (cd == (iconv_t)-1) {
        /* Fallback: simple conversion assuming ASCII */
        for (size_t i = 0; i <= wlen; i++) {
            result[i] = (char)wstr[i];
        }
        return result;
    }

    char* inbuf = (char*)wstr;
    char* outbuf = result;
    size_t inbytesleft = wlen * sizeof(wchar_t);
    size_t outbytesleft = max_len - 1;

    size_t ret = iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
    iconv_close(cd);

    if (ret == (size_t)-1) {
        /* Fallback on error */
        for (size_t i = 0; i <= wlen; i++) {
            result[i] = (char)wstr[i];
        }
    } else {
        *outbuf = '\0';
    }

    return result;
}

/* Convert UTF-8 to wide char */
wchar_t* utf8_to_wchar(const char* str) {
    if (!str) {
        return NULL;
    }

    size_t len = strlen(str);
    size_t max_len = len + 1;

    wchar_t* result = malloc(max_len * sizeof(wchar_t));
    if (!result) {
        return NULL;
    }

    iconv_t cd = iconv_open("WCHAR_T", "UTF-8");
    if (cd == (iconv_t)-1) {
        /* Fallback: simple conversion assuming ASCII */
        for (size_t i = 0; i <= len; i++) {
            result[i] = (wchar_t)str[i];
        }
        return result;
    }

    char* inbuf = (char*)str;
    char* outbuf = (char*)result;
    size_t inbytesleft = len;
    size_t outbytesleft = (max_len - 1) * sizeof(wchar_t);

    size_t ret = iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);
    iconv_close(cd);

    if (ret == (size_t)-1) {
        /* Fallback on error */
        for (size_t i = 0; i <= len; i++) {
            result[i] = (wchar_t)str[i];
        }
    } else {
        result[max_len - 1 - (outbytesleft / sizeof(wchar_t))] = L'\0';
    }

    return result;
}
