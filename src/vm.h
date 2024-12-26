#ifndef __VM_H__
#define __VM_H__

#include <stdint.h>

#include "object.h"
#include "ram.h"
#include "table.h"
#include "value.h"

#define FRAME_MAX 256
#define STACK_MAX FRAME_MAX * UINT8_MAX

typedef struct {
    ObjClosure* closures;
    uint8_t* ip;
    Value* slot;
} CallFrames;

typedef struct {
    CallFrames frames[FRAME_MAX];
    int frame_count;
    Value stack[STACK_MAX];
    Value* stack_top;
    Obj* obj_list;
    Table strings;  // Use hash table as a 'set'.
    Table globals;
    ObjUpvalue* open_upvalues;
} VM;

typedef enum {
    INTERPRET_OK,
    RUNTIME_ERROR,
    COMPILE_ERROR
} PROCESS_RESULT;

typedef enum {
    OP_CONSTANT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,

    OP_NEGATE,
    OP_NOT,

    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,

    OP_PRINT,
    OP_DEFINE_GLOBAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_SET_UPVALUE,
    OP_GET_UPVALUE,

    OP_JUMP_IF_FALSE,
    OP_JUMP,
    OP_BACK_JUMP,

    OP_CLOSURE,
    
    OP_CALL,
    OP_POP,
    OP_CLOSE_UPVALUE,

    OP_RETURN,
} OpCode;

void initVM();
PROCESS_RESULT interpret(const char* source);
void freeVM();
void writeCode(Ram* ram, OpCode op_code);
void writeConstant(Ram* ram, Value val);

#endif // !__VM_H__

