#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "value.h"
#include "object.h"
#include "mem.h"
#include "table.h"
#include "vm.h"

extern VM vm;

void initValueArray(ValueArray* value_array) {
    value_array->val = NULL;
    value_array->count = 0;
    value_array->capacity = 0;
}

void freeValueArray(ValueArray *value_array) {
    if(value_array->val != NULL) {
        FREE(value_array->val, "free value_array\n");
    }
    initValueArray(value_array);
}

void addOne(ValueArray* value_array, Value val) {
    if(value_array->count == value_array->capacity) {
        value_array->capacity = GROW_CAPACITY(value_array->capacity);
        value_array->val = GROW_ARRAY(value_array->val, Value, value_array->count, value_array->capacity);
    }
    value_array->val[value_array->count] = val;
    value_array->count++;
}

static void printOBJ(Value* val) {
    ObjType type = val->as.obj->type;
    switch(type) {
        case OBJ_STRING: {
            printf("%s", AS_CSTRING(*val));
            break;
        }
        case OBJ_FUNCTION: {
            printf("%s", AS_FUNC(*val)->func_name->chars);
            break;
        }
        case OBJ_CLOSURE: {
            printf("%s", AS_CLOSURE(*val)->function->func_name->chars);
        }
    }
};

void printValue(Value* val, const char* pre, const char* tail) {
    ValueType type = val->type;
    switch(type) {
        case NUMBER: {
            printf("%s%g%s", pre, val->as.number, tail);
            break;
        }
        case BOOLEAN: {
            printf("%s", pre);
            printf(val->as.boolean == true ? "true" : "false");
            printf("%s", tail);
            break;
        }
        case NIL: {
            printf("%snil%s", pre, tail);
            break;
        }
        case OBJ: {
            printf("%s", pre);
            printOBJ(val);
            printf("%s", tail);
            break;
        }
    }
}

static uint32_t hashString(const char* initial, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)initial[i];
        hash *= 16777619;
    }
    return hash;
}
