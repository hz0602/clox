#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "vm.h"
#include "table.h"
#include "value.h"
#include "debug.h"
#include "compiler.h"
#include "hint.h"
#include "object.h"

VM vm;

static PROCESS_RESULT runTimeError(const char* mes);
static void addFrame(ObjClosure* closure) {
    if(vm.frame_count == FRAME_MAX) {
        runTimeError("The stack frame is overflox.\n");
    }
    CallFrames* cur = &(vm.frames[vm.frame_count]);
    cur->closures = closure;
    cur->ip = closure->function->ram.code;
    cur->slot = vm.stack_top - closure->function->arity;
    vm.frame_count++;
}

static CallFrames* currentFrame() {
    return &(vm.frames[vm.frame_count - 1]);
}

static Value pop();
static void subtractFrame() {
    int local_count = vm.stack_top - currentFrame()->slot;
    local_count += (currentFrame()->closures->function->type == TYPE_USER ? 1 : 0);
    for(int i = 0; i < local_count; i++) {
        pop();
    }
    vm.frame_count--;
}


static PROCESS_RESULT runTimeError(const char* mes) {
    redHint(mes);
    return RUNTIME_ERROR;
}

static bool push(Value val) {
    int count = vm.stack_top - vm.stack;
    if(count == STACK_MAX) {
        // error hint.
        return false;
    }
   *vm.stack_top = val; 
   vm.stack_top++;
   return true;
}

static Value pop() {
    if(vm.stack_top == vm.stack) {
        runTimeError("The stack is empty, can't pop it,\n");
    }
    vm.stack_top--;
    return *vm.stack_top;
}

void initVM() {
    vm.frame_count = 0;
    vm.stack_top = vm.stack;
    vm.obj_list = NULL;
    initTable(&vm.strings);
    initTable(&vm.globals);
    vm.open_upvalues = NULL;
}

void writeCode(Ram* ram, OpCode op_code) {
    addCode(ram, op_code);
}

// Only 256 elements can be stored.
void writeConstant(Ram* ram, Value val) {
    int index = addConstant(ram, val);
    writeCode(ram, OP_CONSTANT);
    writeCode(ram, index);
}

static void printStack() {
    printf("stack: ");
    Value* cur = vm.stack;
    if(cur == vm.stack_top) {
        printf("[]\n");
        return;
    }
    while(cur < vm.stack_top) {
        printValue(cur, "[", "]\t");
        cur++;
    }
    printf("\n");
}

static void printGlobal() {
    printf("globals: ");
    Table* cur = &vm.globals;
    if(cur->count == 0) {
        printf("[]\n\n");
        return;
    }
    for(int i = 0; i < cur->capacity; i++) {
        if(cur->entry[i].key != NULL) {
            printf("[%s", cur->entry[i].key->chars);
            printValue(&cur->entry[i].val, ":", "], ");
        }
    }
    printf("\n\n");
}

static bool handleCondition(Value val) {
    if(IS_NUMBER(val)) {
        double tmp = AS_NUMBER(val);
        return tmp == 0 ? false : true;
    } else if(IS_NIL(val)) {
        return false;
    }
    return AS_BOOLEAN(val);
}

static ObjUpvalue* captureUpvalue(Value* val) {
    ObjUpvalue* cur = vm.open_upvalues; 
    ObjUpvalue* pre = NULL;
    while(cur != NULL && cur->location > val) {
        pre = cur;
        cur = cur->next;
    }
    if(cur != NULL && cur->location == val) {
        return cur;
    }

    ObjUpvalue* upvalue = allocateObjUpvalue(val);
    upvalue->next = vm.open_upvalues;
    vm.open_upvalues = upvalue;
    return upvalue;
}

static void closeUpvalues(Value* last) {
    ObjUpvalue* cur = vm.open_upvalues;
    while (cur != NULL && cur->location >= last) {
        ObjUpvalue* upvalue = cur;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        cur = upvalue->next;
    }
}

static PROCESS_RESULT run() {
    if(currentFrame()->ip == NULL) {
        printf("No executable instruction.\n");
        return COMPILE_ERROR;
    }
#define READ_BYTE() (*(currentFrame()->ip)++)
#define READ_CONSTANT() currentFrame()->closures->function->ram.constants.val[READ_BYTE()]
#define BINARY_OP(op) \
    do { \
        Value b = pop(); \
        Value a = pop(); \
        if(!IS_NUMBER(a) || !IS_NUMBER(b)) { \
            return runTimeError("Values both aren't 'NUMBER', can't 'BINARY_OP' them.\n"); \
        } \
        if(push(VALUE_NUMBER(AS_NUMBER(a) op AS_NUMBER(b))) == false) { \
            runTimeError("The stack is overflow.\n"); \
        } \
    } while(0)

    for(;;) {
        uint8_t instruction = READ_BYTE();
        switch(instruction) {
            case OP_CONSTANT: {
                Value val = READ_CONSTANT();
                if(push(val) == false) {
                    return runTimeError("The stack is overflow.\n");
                }
                break;
            }
            case OP_NIL: {
                Value val = VALUE_NIL;
                if(push(val) == false) {
                    return runTimeError("The stack is overflow.\n");
                }
                break;
            }
            case OP_TRUE: {
                Value val = VALUE_BOOLEAN(true);
                if(push(val) == false) {
                    return runTimeError("The stack is overflow.\n");
                }
                break;
            }
            case OP_FALSE: {
                Value val = VALUE_BOOLEAN(false);
                if(push(val) == false) {
                    return runTimeError("The stack is overflow.\n");
                }
                break;
            }
            case OP_NEGATE: {
                Value* tmp = vm.stack_top - 1;
                if(!IS_NUMBER(*tmp)) {
                    return runTimeError("The value isn't a 'NUMBER', can't 'OP_NEGATE' it.\n");
                }
                tmp->as.number = -tmp->as.number;
                break;
            }
            case OP_NOT: {
                Value val = pop();
                if(IS_NIL(val) || (IS_BOOLEAN(val) && AS_BOOLEAN(val) == false)) {
                    if(push(VALUE_BOOLEAN(true)) == false) {
                        return runTimeError("The stack is overflow.\n");
                    }
                } else if(IS_BOOLEAN(val) && AS_BOOLEAN(val) == true) {
                    if(push(VALUE_BOOLEAN(false)) == false) {
                        return runTimeError("The stack is overflow.\n");
                    }
                } else {
                    return runTimeError("The value isn't a 'BOOLEAN' or 'NIL', can't 'OP_NOT' it.\n");
                }
                break;
            }
            case OP_ADD: {
                Value b = pop();
                Value a = pop();
                if(IS_STRING(a) && IS_STRING(b)) {
                    // strcat
                    char* str1 = AS_CSTRING(a);
                    int length1 = strlen(str1);
                    char* str2 = AS_CSTRING(b);
                    int length2 = strlen(str2);
                    int length = length1 + length2;
                    char total_str[length + 1];
                    strncpy(total_str, str1, length1);
                    strncpy(total_str + length1, str2, length2);
                    total_str[length] = '\0';
                    if(push(allocateString(total_str, length)) == false) {
                        return runTimeError("The stack is overflow.\n");
                    }
                } else if(IS_NUMBER(a) && IS_NUMBER(b)) {
                    if(push(VALUE_NUMBER(AS_NUMBER(a) + AS_NUMBER(b))) == false) {
                        return runTimeError("The stack is overflow.\n");
                    }
                } else {
                    return runTimeError("Values both aren't 'NUMBER' or 'STRING', can't 'OP_ADD' them.\n"); \
                }
                break;
            }
            case OP_SUBTRACT: {
                BINARY_OP(-);
                break;
            }
            case OP_MULTIPLY: {
                BINARY_OP(*);
                break;
            }
            case OP_DIVIDE: {
                BINARY_OP(/);
                break;
            }
            case OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                bool res;
                if(IS_NUMBER(a) && IS_NUMBER(b)) {
                    res = (AS_NUMBER(a) == AS_NUMBER(b));
                } else if(IS_BOOLEAN(a) && IS_BOOLEAN(b)) {
                    res = (AS_BOOLEAN(a) == AS_BOOLEAN(b));
                } else if(IS_STRING(a) && IS_STRING(b)) {
                    res = (AS_STRING(a) == AS_STRING(b));
                } else if(IS_NIL(a) && IS_NIL(b)) {
                    res = true;
                } else {
                    return runTimeError("The types of values aren't the same, can't 'OP_EQUAL' them.\n");
                }
                if(push(VALUE_BOOLEAN(res)) == false) {
                    return runTimeError("The stack is overflow.\n");
                }
                break;
            }
            case OP_GREATER: {
                Value b = pop();
                Value a = pop();
                if(!IS_NUMBER(a) || !IS_NUMBER(b)) {
                    return runTimeError("values both aren't NUMBER, can't 'OP_GREATER' them.\n");
                }
                if(push(VALUE_BOOLEAN(AS_NUMBER(a) > AS_NUMBER(b))) == false) {
                    return runTimeError("The stack is overflow.\n");
                }
                break;
            }
            case OP_LESS: {
                Value b = pop();
                Value a = pop();
                if(!IS_NUMBER(a) || !IS_NUMBER(b)) {
                    return runTimeError("values both aren't NUMBER, can't 'OP_LESS' them.\n");
                }
                if(push(VALUE_BOOLEAN(AS_NUMBER(a) < AS_NUMBER(b))) == false) {
                    return runTimeError("The stack is overflow.\n");
                }
                break;
            }
            case OP_PRINT: {
                Value val = pop();
                printValue(&val, "", "\n");
                break;
            }
            case OP_DEFINE_GLOBAL: {
                Value key = READ_CONSTANT();
                Value val = pop();
                tableSet(&vm.globals, AS_STRING(key), val);
                break;
            }
            case OP_GET_GLOBAL: {
                Value key = READ_CONSTANT();
                Value val;
                if(!tableGet(&vm.globals, AS_STRING(key), &val)) {
                    return runTimeError("Not find the global variable.\n");
                };
                if(push(val) == false) {
                    return runTimeError("The stack is overflow.\n");
                }
                break;
            }
            case OP_SET_GLOBAL: {
                Value key = READ_CONSTANT();
                Value val = pop();
                if(tableSet(&vm.globals, AS_STRING(key), val)) {
                    tableDelete(&vm.globals, AS_STRING(key));
                    return runTimeError("Can't find the variable name.\n");
                }
                if(push(val) == false) {
                    return runTimeError("The stack is overflow.\n");
                }
                break;
            }
            case OP_GET_LOCAL: {
                uint8_t slot = READ_BYTE();
                if(push(currentFrame()->slot[slot]) == false) {
                    return runTimeError("The stack is overflow.\n");
                }
                break;
            }
            case OP_SET_LOCAL: {
                uint8_t slot = READ_BYTE();
                Value* val = &(currentFrame()->slot[slot]);
                *val = pop();
                if(push(*val) == false) {
                    return runTimeError("The stack is overflow.\n");
                }
                break;
            }
            case OP_JUMP_IF_FALSE: {
                Value condition = *(vm.stack_top - 1);
                // Value condition = pop();
                uint8_t low_bits = READ_BYTE();
                uint8_t high_bits = READ_BYTE();
                if(handleCondition(condition) == false) {
                    currentFrame()->ip += (uint16_t)((high_bits << 8) + low_bits);
                }
                break;
            }
            case OP_JUMP: {
                uint8_t low_bits = READ_BYTE();
                uint8_t high_bits = READ_BYTE();
                currentFrame()->ip += (uint16_t)((high_bits << 8) + low_bits);
                break;
            }
            case OP_BACK_JUMP: {
                uint8_t low_bits = READ_BYTE();
                uint8_t high_bits = READ_BYTE();
                currentFrame()->ip -= (uint16_t)((high_bits << 8) + low_bits);
                break;
            }
            case OP_CALL: {
                uint8_t arg_num = READ_BYTE();
                Value* call_func = vm.stack_top - arg_num - 1;
                if(!IS_CLOSURE(*call_func)) {
                    return runTimeError("The value can't be called.\n");
                }
                ObjClosure* closure = (AS_CLOSURE(*call_func));
                if(arg_num != closure->function->arity) {
                    return runTimeError("The number of parameters is wrong.\n");
                }
                addFrame(closure);
                break;
            }
            case OP_CLOSURE: {
                ObjClosure* closure = allocateObjClosure((ObjFunction*)(READ_CONSTANT().as.obj));
                push(VALUE_OBJ(closure));
                for(int i = 0; i < closure->function->upvalue_count; i++) {
                    int is_local = READ_BYTE();
                    int index = READ_BYTE();
                    if(is_local) {
                        closure->upvalues[i] = captureUpvalue(currentFrame()->slot + index);
                    } else {
                        closure->upvalues[i] = currentFrame()->closures->upvalues[index];
                    }
                }
                break;
            }
            case OP_GET_UPVALUE: {
                int index = READ_BYTE();
                push(*(currentFrame()->closures->upvalues[index]->location));
                break;
            }
            case OP_SET_UPVALUE: {
                Value val = pop();
                int index = READ_BYTE();
                *(currentFrame()->closures->upvalues[index]->location) = val;
                break;
            }
            case OP_POP: {
                pop();
                break;
            }
            case OP_CLOSE_UPVALUE: {
                closeUpvalues(vm.stack_top - 1);
                pop();
                break;
            }
            case OP_RETURN: {
                Value return_value = pop();
                closeUpvalues(currentFrame()->slot);
                if(vm.frame_count > 1) {
                    subtractFrame();
                    push(return_value);
                    break;
                }
                return INTERPRET_OK;
            }
        }
        // printStack();
        // printGlobal();
    }
    return RUNTIME_ERROR;

#undef BINARY_OP
#undef READ_CONSTANT
#undef READ_BYTE
}

static void disassembleAll() {
    for(int i = 0; i < vm.globals.capacity; i++) {
        Value* tmp = &vm.globals.entry[i].val;
        if(IS_CLOSURE(*tmp)) {
            disassembleFunction(AS_CLOSURE(*tmp)->function);
        }
    }
    Value* cur = vm.stack;
    while(cur != vm.stack_top) {
        if(IS_CLOSURE(*cur)) {
            disassembleFunction(AS_CLOSURE(*cur)->function);
        }
        cur++;
    }
}

static void vmStateCheck() {
    if(vm.stack_top > vm.stack) {
        redHint("There are stack values have't be pop.\n");
        printStack();
    } else if(vm.stack_top < vm.stack) {
        redHint("The stack is wrong.\n");
    }
    if(vm.frame_count != 0) {
        redHint("The number of frame in vm is wrong.\n");
    }
}

PROCESS_RESULT interpret(const char* source) {
    initVM();

    ObjFunction* main_func = compile(source);
    if(main_func == NULL) {
        return COMPILE_ERROR;
    }

    addFrame(allocateObjClosure(main_func));
    PROCESS_RESULT res = run();
    disassembleFunction(currentFrame()->closures->function);
    subtractFrame();
    disassembleAll();
    vmStateCheck();

    return res;
}

void freeVM() {
    freeObjects();
    freeTable(&vm.strings);
    freeTable(&vm.globals);
}
