#ifndef __COMPILER_H__
#define __COMPILER_H__

#include <stdbool.h>
#include "value.h"

ObjFunction* compile(const char* source);
void justScan(const char* source);

#endif // !__COMPILER_H__
