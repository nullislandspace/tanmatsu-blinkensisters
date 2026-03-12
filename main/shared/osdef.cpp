#include "osdef.h"
#include <string.h>
#include <stdlib.h>

char* BS_strdup(const char* s) {
    char* result = (char*)malloc(strlen(s) + 1);
    if (result == NULL) return NULL;
    strcpy(result, s);
    return result;
}
