#include <stdlib.h>
#include <string.h>
#include "ram.h"
#include "value.h"
#include "vm.h"
#include "object.h"
#include "mem.h"

extern VM vm;

static Obj* allocateObj(ObjType type) {
    Obj* res;
    switch(type) {
        case OBJ_STRING: {
            res = (Obj*)malloc(sizeof(ObjString));
            break;
        }
        case OBJ_FUNCTION: {
            res = (Obj*)malloc(sizeof(ObjFunction));
            break;
        }
        case OBJ_CLOSURE: {
            res = (Obj*)malloc(sizeof(ObjClosure));
            break;
        }
        case OBJ_UPVALUE: {
            res = (Obj*)malloc(sizeof(ObjUpvalue));
            break;
        }
    }
    res->type = type;
    res->next = vm.obj_list;
    vm.obj_list = res;
    return res;
}

static uint32_t hashString(const char* initial, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)initial[i];
        hash *= 16777619;
    }
    return hash;
}

static void freeObject(Obj* obj) {
    switch(obj->type) {
        case OBJ_STRING: {
            // printf("free '%s'\n", ((ObjString*)obj)->chars);
            FREE(((ObjString*)obj)->chars, "free ObjString->chars\n");
            FREE(obj, "free ObjString\n");
            break;
        }
        case OBJ_FUNCTION: {
            freeRam(&(((ObjFunction*)obj)->ram));
            FREE(obj, "free ObjFunction\n");
            break;
        }
        case OBJ_CLOSURE: {
            FREE(obj, "free ObjClosure\n") ;
            break;
        }
        case OBJ_UPVALUE: {
            // FREE(((ObjUpvalue*)obj)->location, "free Value\n");
            FREE(obj, "free ObjUpvalue\n") ;
            break;
        }
    }
    obj = NULL;
}

void freeObjects() {
    Obj* tmp = vm.obj_list;
    while(tmp != NULL) {
        Obj* next = tmp->next;
        freeObject(tmp);
        tmp = next;
    }
}

ObjString* allocateObjString(const char* initial, int length) {
    uint32_t hash = hashString(initial, length);

    ObjString* str = tableFindString(&vm.strings, initial, length, hash);
    if(str != NULL) {
        return str;
    }
    str = (ObjString*)allocateObj(OBJ_STRING);
    str->chars = (char*)malloc(sizeof(char) * (length + 1));
    strncpy(str->chars, initial, length);
    str->chars[length] = '\0';
    str->length = length;
    str->hash_code = hash;
    return str;
}

Value allocateString(const char* initial, int length) {
    ObjString* str = allocateObjString(initial, length);
    tableSet(&vm.strings, str, VALUE_NIL);
    return VALUE_OBJ(str);
}

ObjFunction* allocateObjFunction(FunctionType type) {
    ObjFunction* func = (ObjFunction*)allocateObj(OBJ_FUNCTION);
    func->arity = 0; 
    func->upvalue_count = 0;
    func->func_name = NULL;
    func->type = type;
    initRam(&func->ram);
    return func;
}

ObjClosure* allocateObjClosure(ObjFunction* func) {
    ObjClosure* closure = (ObjClosure*)allocateObj(OBJ_CLOSURE);
    closure->function = func;
    closure->upvalues = (ObjUpvalue**)malloc(func->upvalue_count * sizeof(ObjUpvalue*));
    return closure;
}

ObjUpvalue* allocateObjUpvalue(Value* val) {
    ObjUpvalue* upvalue = (ObjUpvalue*)allocateObj(OBJ_UPVALUE);
    upvalue->location = val;
    upvalue->closed = VALUE_NIL;
    upvalue->next = NULL;
    return upvalue;
}
