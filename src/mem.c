#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mem.h"

void* growArray(void* array, int size, int count, int capacity) {
    void* res = malloc(size * capacity);
    for(int i = 0; i < count * size; i += size) {
        memcpy(res + i, array + i, size);
    }
    if(array != NULL) FREE(array, "free array\n");
    return res;
}

void FREE(void* ptr, const char* message) {
    // printf("%s", message);
    free(ptr);
}
