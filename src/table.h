#ifndef __TABLE_H__
#define __TABLE_H__

#include "value.h"

typedef struct {
    ObjString* key;
    Value val;
} Entry;

typedef struct {
    int count;
    int capacity;
    Entry* entry;
} Table;

void initTable(Table* table);
void freeTable(Table* table);
bool tableSet(Table* table, ObjString* key, Value val);
bool tableGet(Table* table, ObjString* key, Value* val);
bool tableDelete(Table* table, ObjString* key);
ObjString* tableFindString(Table* table, const char* initial, int length, uint32_t hash);

#endif // !__TABLE_H__

