#ifndef __RAM_H__
#define __RAM_H__

#include <stdint.h>
#include "value.h"

typedef struct {
    int count; 
    int capacity;
    uint8_t* code; 
    ValueArray constants;
} Ram;

void initRam(Ram* ram);
void freeRam(Ram* ram);
void addCode(Ram* ram, uint8_t code);
int addConstant(Ram* ram, Value val);

#endif // !__RAM_H__

