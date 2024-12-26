#ifndef __OBJECT_H__
#define __OBJECT_H__

#include <stdint.h>
#include "ram.h"
#include "value.h"

#define AS_STRING(value) ((ObjString*)((value).as.obj))
#define AS_CSTRING(value) ((char*)(AS_STRING((value))->chars))
#define AS_FUNC(value) ((ObjFunction*)((value).as.obj))
#define AS_CLOSURE(value) ((ObjClosure*)((value).as.obj))

#define IS_STRING(value) (IS_OBJ((value)) && (value).as.obj->type == OBJ_STRING)
#define IS_FUNC(value) (IS_OBJ((value)) && (value).as.obj->type == OBJ_FUNCTION)
#define IS_CLOSURE(value) (IS_OBJ((value)) && (value).as.obj->type == OBJ_CLOSURE)

typedef enum {
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_CLOSURE,
    OBJ_UPVALUE,
} ObjType;

struct Obj{
    ObjType type;
    struct Obj* next;
};

struct ObjString {
    Obj obj;
    int length;
    char* chars;
    uint32_t hash_code;
};

typedef enum {
    TYPE_MAIN,
    TYPE_USER,
} FunctionType;

struct ObjFunction {
    Obj obj;
    int arity;
    int upvalue_count;
    ObjString* func_name;
    FunctionType type;
    Ram ram;
};

typedef struct ObjUpvalue {
    Obj obj;
    Value* location;    // Point to an upvalue or a real value.
    Value closed;
    struct ObjUpvalue* next;
} ObjUpvalue;

struct ObjClosure {
    Obj obj;
    ObjFunction* function;
    ObjUpvalue** upvalues;
};

void freeObjects();
ObjString* allocateObjString(const char* initial, int length);
Value allocateString(const char* initial, int length);
ObjFunction* allocateObjFunction(FunctionType type);
ObjClosure* allocateObjClosure(ObjFunction* func);
ObjUpvalue* allocateObjUpvalue(Value* val);

#endif // ! __OBJECT_H__

