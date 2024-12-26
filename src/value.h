#ifndef __VALUE_H__
#define __VALUE_H__

#include <stdbool.h>
#include <stdint.h>

typedef struct Obj Obj;
typedef struct ObjString ObjString;
typedef struct ObjFunction ObjFunction;
typedef struct ObjClosure ObjClosure;

#define VALUE_NUMBER(value)     (Value){NUMBER, {.number=(value)}}
#define VALUE_NIL               (Value){NIL, {.number=0}}
#define VALUE_BOOLEAN(value)    (Value){BOOLEAN, {.boolean=(value)}}
#define VALUE_OBJ(value)        (Value){OBJ, {.obj=(Obj*)(value)}}

#define AS_NUMBER(a) (double)(a.as.number)
#define AS_BOOLEAN(a) (bool)(a.as.boolean)

#define IS_NUMBER(val) ((val).type == NUMBER)
#define IS_NIL(val) ((val).type == NIL)
#define IS_BOOLEAN(val) ((val).type == BOOLEAN)
#define IS_OBJ(val) ((val).type == OBJ)

typedef enum {
    NIL,
    NUMBER,
    BOOLEAN,
    OBJ
} ValueType;

typedef struct {
    ValueType type;
    union {
        double number;
        bool boolean;
        Obj* obj;
    } as;
} Value;

typedef struct {
    int count;
    int capacity;
    Value* val;
} ValueArray;

void initValueArray(ValueArray* value_array);
void freeValueArray(ValueArray* value_array);
void addOne(ValueArray* value_array, Value val);
void printValue(Value* val, const char* pre, const char* tail);

#endif // !__VALUE_H__

