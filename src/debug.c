#include <stdint.h>
#include <stdio.h>
#include "debug.h"
#include "object.h"
#include "value.h"
#include "vm.h"

extern VM vm;

static int simpleInstruction(const char* mes, Ram* ram, int offset) {
    printf("%s\n", mes);
    return offset + 1;
}

static int constantInstruction(const char* mes, Ram* ram, int offset) {
    int constant_index = ram->code[offset + 1];
    printf("%s\t%d ", mes, ram->code[offset + 1]);

    printValue(&ram->constants.val[constant_index], "\"", "\"\n");

    return offset + 2;
}

static int variableInstruction(const char* mes, Ram* ram, int offset) {
    printf("%s\t%d\n", mes, ram->code[offset + 1]);
    return offset + 2;
}

static int jumpInstruction(const char* mes, Ram* ram, int offset, bool is_back) {
    uint16_t jump_offset = (ram->code[offset + 2] << 8) + ram->code[offset + 1];
    if(is_back) {
        jump_offset -= 3;
    } else {
        jump_offset += 3;
    }
    printf("%s\t%d\n", mes, jump_offset);
    return offset + 3;
}

static int closureInstruction(const char* mes, Ram* ram, int offset) {
    int constant_index = ram->code[offset + 1];
    printf("%s\t%d\n", mes, constant_index);
    return offset + 2 * (AS_FUNC(ram->constants.val[constant_index])->upvalue_count + 1);
}

int disassembleInstruction(ObjFunction* func, int offset) {
    printf("\033[1;31m%04d\t\033[0m", offset);
    Ram* ram = &func->ram;
    switch(ram->code[offset]) {
        case OP_CONSTANT: {
            return constantInstruction("OP_CONSTANT", ram, offset);
        }
        case OP_NIL: {
            return simpleInstruction("OP_NIL", ram, offset);
        }
        case OP_TRUE: {
            return simpleInstruction("OP_TRUE", ram, offset);
        }
        case OP_FALSE: {
            return simpleInstruction("OP_FALSE", ram, offset);
        }
        case OP_NEGATE: {
            return simpleInstruction("OP_NEGATE", ram, offset);
        }
        case OP_NOT: {
            return simpleInstruction("OP_NOT", ram, offset);
        }
        case OP_EQUAL: {
            return simpleInstruction("OP_EQUAL", ram, offset);
        }
        case OP_ADD: {
            return simpleInstruction("OP_ADD", ram, offset);
        }
        case OP_SUBTRACT: {
            return simpleInstruction("OP_SUBTRACT", ram, offset);
        }
        case OP_MULTIPLY: {
            return simpleInstruction("OP_MULTIPLY", ram, offset);
        }
        case OP_DIVIDE: {
            return simpleInstruction("OP_DIVIDE", ram, offset);
        }
        case OP_GREATER: {
            return simpleInstruction("OP_GREATER", ram, offset);
        }
        case OP_LESS: {
            return simpleInstruction("OP_LESS", ram, offset);
        }
        case OP_PRINT: {
            return simpleInstruction("OP_PRINT", ram, offset);
        }
        case OP_DEFINE_GLOBAL: {
            return constantInstruction("OP_DEFINE_GLOBAL", ram, offset);
        }
        case OP_GET_GLOBAL: {
            return constantInstruction("OP_GET_GLOBAL", ram, offset);
        }
        case OP_SET_GLOBAL: {
            return constantInstruction("OP_SET_GLOBAL", ram, offset);
        }
        case OP_GET_LOCAL: {
            return variableInstruction("OP_GET_LOCAL", ram, offset);
        }
        case OP_SET_LOCAL: {
            return variableInstruction("OP_SET_LOCAL", ram, offset);
        }
        case OP_GET_UPVALUE: {
            return variableInstruction("OP_GET_UPVALUE", ram, offset);
        }
        case OP_SET_UPVALUE: {
            return variableInstruction("OP_SET_UPVALUE", ram, offset);
        }
        case OP_JUMP_IF_FALSE: {
            return jumpInstruction("OP_JUMP_IF_FALSE", ram, offset, false);
        }
        case OP_JUMP: {
            return jumpInstruction("OP_JUMP", ram, offset, false);
        }
        case OP_BACK_JUMP: {
            return jumpInstruction("OP_BACK_JUMP", ram, offset, true);
        }
        case OP_CALL: {
            // return variableInstruction("OP_CALL", ram, offset);
            return variableInstruction("OP_CALL", ram, offset);
        }
        case OP_CLOSURE: {
            return closureInstruction("OP_CLOSURE", ram, offset);
        }
        case OP_POP: {
            return simpleInstruction("OP_POP", ram, offset);
        }
        case OP_CLOSE_UPVALUE: {
            return simpleInstruction("OP_CLOSE_UPVALUE", ram, offset);
        }
        case OP_RETURN: {
            return simpleInstruction("OP_RETURN", ram, offset);
        }
    }
    return -1;
}

void disassembleFunction(ObjFunction* function) {
    if(function->type == TYPE_MAIN) {
        printf("********** script **********\n");
    } else {
        printf("********** %s **********\n", function->func_name->chars);
    }
    for(int i = 0; i < function->ram.count;) {
        i = disassembleInstruction(function, i);
    }

    printf("****************************\n");
}
