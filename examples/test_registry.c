#include <stdio.h>
#include <string.h>
#include "../include/winreg.h"

void print_error(const char* operation, LONG error) {
    printf("%s failed with error code: %ld\n", operation, error);
}

int main(void) {
    HKEY hKey;
    LONG result;
    DWORD disposition;

    printf("Windows Registry API Test on Linux\n");
    printf("====================================\n\n");

    /* Test 1: Create a key */
    printf("Test 1: Creating a new registry key...\n");
    result = RegCreateKeyExA(
        HKEY_LOCAL_MACHINE,
        "Software\\TestCompany\\TestApp",
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,
        &hKey,
        &disposition
    );

    if (result == ERROR_SUCCESS) {
        printf("Success! Key %s\n",
               disposition == REG_CREATED_NEW_KEY ? "created" : "opened");
    } else {
        print_error("RegCreateKeyEx", result);
        return 1;
    }

    /* Test 2: Set string value */
    printf("\nTest 2: Setting a string value...\n");
    const char* string_value = "Hello, Registry!";
    result = RegSetValueExA(
        hKey,
        "TestString",
        0,
        REG_SZ,
        (const BYTE*)string_value,
        strlen(string_value) + 1
    );

    if (result == ERROR_SUCCESS) {
        printf("Success! String value set\n");
    } else {
        print_error("RegSetValueEx (string)", result);
    }

    /* Test 3: Set DWORD value */
    printf("\nTest 3: Setting a DWORD value...\n");
    DWORD dword_value = 12345;
    result = RegSetValueExA(
        hKey,
        "TestDWORD",
        0,
        REG_DWORD,
        (const BYTE*)&dword_value,
        sizeof(DWORD)
    );

    if (result == ERROR_SUCCESS) {
        printf("Success! DWORD value set to %u\n", dword_value);
    } else {
        print_error("RegSetValueEx (DWORD)", result);
    }

    /* Test 4: Set binary value */
    printf("\nTest 4: Setting a binary value...\n");
    BYTE binary_data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    result = RegSetValueExA(
        hKey,
        "TestBinary",
        0,
        REG_BINARY,
        binary_data,
        sizeof(binary_data)
    );

    if (result == ERROR_SUCCESS) {
        printf("Success! Binary value set\n");
    } else {
        print_error("RegSetValueEx (binary)", result);
    }

    /* Close the key */
    RegCloseKey(hKey);

    /* Test 5: Re-open the key and read values */
    printf("\nTest 5: Re-opening key and reading values...\n");
    result = RegOpenKeyExA(
        HKEY_LOCAL_MACHINE,
        "Software\\TestCompany\\TestApp",
        0,
        KEY_READ,
        &hKey
    );

    if (result != ERROR_SUCCESS) {
        print_error("RegOpenKeyEx", result);
        return 1;
    }

    /* Read string value */
    char string_buffer[256];
    DWORD buffer_size = sizeof(string_buffer);
    DWORD value_type;

    result = RegQueryValueExA(
        hKey,
        "TestString",
        NULL,
        &value_type,
        (LPBYTE)string_buffer,
        &buffer_size
    );

    if (result == ERROR_SUCCESS) {
        printf("String value: %s (type: %u)\n", string_buffer, value_type);
    } else {
        print_error("RegQueryValueEx (string)", result);
    }

    /* Read DWORD value */
    DWORD read_dword;
    buffer_size = sizeof(DWORD);

    result = RegQueryValueExA(
        hKey,
        "TestDWORD",
        NULL,
        &value_type,
        (LPBYTE)&read_dword,
        &buffer_size
    );

    if (result == ERROR_SUCCESS) {
        printf("DWORD value: %u (type: %u)\n", read_dword, value_type);
    } else {
        print_error("RegQueryValueEx (DWORD)", result);
    }

    /* Read binary value */
    BYTE read_binary[256];
    buffer_size = sizeof(read_binary);

    result = RegQueryValueExA(
        hKey,
        "TestBinary",
        NULL,
        &value_type,
        read_binary,
        &buffer_size
    );

    if (result == ERROR_SUCCESS) {
        printf("Binary value (%u bytes): ", buffer_size);
        for (DWORD i = 0; i < buffer_size; i++) {
            printf("%02X ", read_binary[i]);
        }
        printf("(type: %u)\n", value_type);
    } else {
        print_error("RegQueryValueEx (binary)", result);
    }

    /* Test 6: Enumerate values */
    printf("\nTest 6: Enumerating all values...\n");
    DWORD index = 0;
    char value_name[256];
    DWORD name_size;
    BYTE value_data[256];
    DWORD data_size;

    while (TRUE) {
        name_size = sizeof(value_name);
        data_size = sizeof(value_data);

        result = RegEnumValueA(
            hKey,
            index,
            value_name,
            &name_size,
            NULL,
            &value_type,
            value_data,
            &data_size
        );

        if (result == ERROR_NO_MORE_ITEMS) {
            break;
        }

        if (result == ERROR_SUCCESS) {
            printf("  [%u] Name: %s, Type: %u, Size: %u bytes\n",
                   index, value_name, value_type, data_size);
        } else {
            print_error("RegEnumValue", result);
            break;
        }

        index++;
    }

    printf("Total values: %u\n", index);

    /* Test 7: Create subkey */
    printf("\nTest 7: Creating a subkey...\n");
    HKEY hSubKey;
    result = RegCreateKeyExA(
        hKey,
        "SubKey1",
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_ALL_ACCESS,
        NULL,
        &hSubKey,
        &disposition
    );

    if (result == ERROR_SUCCESS) {
        printf("Success! Subkey created\n");

        /* Set a value in the subkey */
        const char* subkey_value = "I'm in a subkey!";
        RegSetValueExA(
            hSubKey,
            "SubValue",
            0,
            REG_SZ,
            (const BYTE*)subkey_value,
            strlen(subkey_value) + 1
        );

        RegCloseKey(hSubKey);
    } else {
        print_error("RegCreateKeyEx (subkey)", result);
    }

    /* Test 8: Enumerate subkeys */
    printf("\nTest 8: Enumerating subkeys...\n");
    index = 0;
    char subkey_name[256];

    while (TRUE) {
        name_size = sizeof(subkey_name);

        result = RegEnumKeyExA(
            hKey,
            index,
            subkey_name,
            &name_size,
            NULL,
            NULL,
            NULL,
            NULL
        );

        if (result == ERROR_NO_MORE_ITEMS) {
            break;
        }

        if (result == ERROR_SUCCESS) {
            printf("  [%u] %s\n", index, subkey_name);
        } else {
            print_error("RegEnumKeyEx", result);
            break;
        }

        index++;
    }

    printf("Total subkeys: %u\n", index);

    /* Test 9: Query key information */
    printf("\nTest 9: Querying key information...\n");
    DWORD subkey_count, max_subkey_len, value_count, max_value_name_len, max_value_len;

    result = RegQueryInfoKeyA(
        hKey,
        NULL,
        NULL,
        NULL,
        &subkey_count,
        &max_subkey_len,
        NULL,
        &value_count,
        &max_value_name_len,
        &max_value_len,
        NULL,
        NULL
    );

    if (result == ERROR_SUCCESS) {
        printf("Subkeys: %u (max name length: %u)\n", subkey_count, max_subkey_len);
        printf("Values: %u (max name length: %u, max data length: %u)\n",
               value_count, max_value_name_len, max_value_len);
    } else {
        print_error("RegQueryInfoKey", result);
    }

    RegCloseKey(hKey);

    /* Test 10: Delete value */
    printf("\nTest 10: Deleting a value...\n");
    result = RegOpenKeyExA(
        HKEY_LOCAL_MACHINE,
        "Software\\TestCompany\\TestApp",
        0,
        KEY_ALL_ACCESS,
        &hKey
    );

    if (result == ERROR_SUCCESS) {
        result = RegDeleteValueA(hKey, "TestBinary");
        if (result == ERROR_SUCCESS) {
            printf("Success! Value deleted\n");
        } else {
            print_error("RegDeleteValue", result);
        }
        RegCloseKey(hKey);
    }

    /* Test 11: Delete tree (cleanup) */
    printf("\nTest 11: Cleaning up (deleting entire tree)...\n");
    result = RegDeleteTreeA(
        HKEY_LOCAL_MACHINE,
        "Software\\TestCompany"
    );

    if (result == ERROR_SUCCESS) {
        printf("Success! Tree deleted\n");
    } else {
        print_error("RegDeleteTree", result);
    }

    printf("\nAll tests completed!\n");
    return 0;
}
