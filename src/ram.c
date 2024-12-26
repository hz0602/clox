#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "ram.h"
#include "mem.h"
#include "value.h"

void initRam(Ram* ram) {
    ram->count = 0;
    ram->capacity = 0;
    ram->code = NULL;
    initValueArray(&ram->constants);
}

void freeRam(Ram* ram) {
    if(ram->code != NULL) {
        FREE(ram->code, "free ram->code\n");
    }
    freeValueArray(&ram->constants);
}

void addCode(Ram* ram, uint8_t code) {
    if(ram->count == ram->capacity) {
        ram->capacity = GROW_CAPACITY(ram->capacity);
        ram->code = GROW_ARRAY(ram->code, uint8_t, ram->count, ram->capacity);
    }
    ram->code[ram->count] = code;
    ram->count++;
}

int addConstant(Ram* ram, Value val) {
    addOne(&ram->constants, val);
    return ram->constants.count - 1;
}

