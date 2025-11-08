#ifndef _WINNT_H
#define _WINNT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <wchar.h>

/* Basic Types */
typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;
typedef uint64_t QWORD;
typedef int32_t LONG;
typedef uint64_t ULONG64;
typedef int64_t LONGLONG;
typedef uint64_t ULONGLONG;
typedef void* PVOID;
typedef void* LPVOID;
typedef const void* LPCVOID;
typedef char* LPSTR;
typedef const char* LPCSTR;
typedef wchar_t* LPWSTR;
typedef const wchar_t* LPCWSTR;
typedef BYTE* LPBYTE;
typedef DWORD* LPDWORD;
typedef int BOOL;

/* Pointer-sized types */
#if defined(__LP64__) || defined(_WIN64)
typedef uint64_t ULONG_PTR;
typedef int64_t LONG_PTR;
#else
typedef uint32_t ULONG_PTR;
typedef int32_t LONG_PTR;
#endif

/* TCHAR mapping - default to ANSI */
#ifndef UNICODE
typedef char TCHAR;
typedef LPSTR LPTSTR;
typedef LPCSTR LPCTSTR;
#else
typedef wchar_t TCHAR;
typedef LPWSTR LPTSTR;
typedef LPCWSTR LPCTSTR;
#endif

/* Boolean values */
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

/* Access Rights */
#define DELETE                   0x00010000L
#define READ_CONTROL             0x00020000L
#define WRITE_DAC                0x00040000L
#define WRITE_OWNER              0x00080000L
#define SYNCHRONIZE              0x00100000L
#define STANDARD_RIGHTS_REQUIRED 0x000F0000L
#define STANDARD_RIGHTS_READ     READ_CONTROL
#define STANDARD_RIGHTS_WRITE    READ_CONTROL
#define STANDARD_RIGHTS_EXECUTE  READ_CONTROL
#define STANDARD_RIGHTS_ALL      0x001F0000L
#define SPECIFIC_RIGHTS_ALL      0x0000FFFFL

/* Security Attributes */
typedef struct _SECURITY_ATTRIBUTES {
    DWORD nLength;
    LPVOID lpSecurityDescriptor;
    BOOL bInheritHandle;
} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

/* File Time */
typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;

#ifdef __cplusplus
}
#endif

#endif /* _WINNT_H */
