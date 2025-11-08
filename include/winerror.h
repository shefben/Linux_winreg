#ifndef _WINERROR_H
#define _WINERROR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "winnt.h"

/* Success and Error codes */
#define ERROR_SUCCESS                    0L
#define ERROR_INVALID_FUNCTION           1L
#define ERROR_FILE_NOT_FOUND             2L
#define ERROR_PATH_NOT_FOUND             3L
#define ERROR_ACCESS_DENIED              5L
#define ERROR_INVALID_HANDLE             6L
#define ERROR_NOT_ENOUGH_MEMORY          8L
#define ERROR_INVALID_PARAMETER          87L
#define ERROR_INSUFFICIENT_BUFFER        122L
#define ERROR_MORE_DATA                  234L
#define ERROR_NO_MORE_ITEMS              259L

/* Registry specific errors */
#define ERROR_BADDB                      1009L
#define ERROR_BADKEY                     1010L
#define ERROR_CANTOPEN                   1011L
#define ERROR_CANTREAD                   1012L
#define ERROR_CANTWRITE                  1013L
#define ERROR_REGISTRY_RECOVERED         1014L
#define ERROR_REGISTRY_CORRUPT           1015L
#define ERROR_REGISTRY_IO_FAILED         1016L
#define ERROR_NOT_REGISTRY_FILE          1017L
#define ERROR_KEY_DELETED                1018L
#define ERROR_KEY_HAS_CHILDREN           1020L
#define ERROR_CHILD_MUST_BE_VOLATILE     1021L

#ifdef __cplusplus
}
#endif

#endif /* _WINERROR_H */
