#ifndef __DEBUG_H__
#define __DEBUG_H__

#include "value.h"

void disassembleFunction(ObjFunction* function);
int disassembleInstruction(ObjFunction* func, int offset);


#endif // !__DEBUG_H__
