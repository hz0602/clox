#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "table.h"
#include "mem.h"
#include "value.h"
#include "object.h"

#define MAX_LOAD 0.75

#define IS_TOMBSTONE(entry) (entry->key == NULL && entry->val.type == BOOLEAN && entry->val.as.boolean == true)
#define IS_EMPTY_ENTRY(entry) (entry->key == NULL && entry->val.type == NIL)
#define SET_TOMBSTONE(entry) \
                    do { \
                        entry->key = NULL; \
                        entry->val = VALUE_BOOLEAN(true); \
                    } while(0)


void initTable(Table* table) {
    table->count = 0;
    table->capacity = 0;
    table->entry = NULL;
}

void freeTable(Table* table) {
    if(table->entry) FREE(table->entry, "free table->entry\n");
    initTable(table);
}

static Entry* findEntry(Entry* src_entry, ObjString* key, int capacity) {
    int index = key->hash_code % capacity;

    Entry* tombstone = NULL;
    for(;;) {
        Entry* entry = &src_entry[index];

        if(IS_TOMBSTONE(entry)) {
            tombstone = entry;
        } else if(IS_EMPTY_ENTRY(entry)){
            return tombstone == NULL ? entry : tombstone;
        } else if(entry->key == key) {
            return entry;
        }
        index = (index + 1) % capacity;
    }
}

static void adjustTable(Table* table, int capacity) {
    Entry* new_entry = (Entry*)malloc(sizeof(Entry) * capacity);
    for(int i = 0; i < capacity; i++) {
        new_entry[i].key = NULL;
        new_entry[i].val = VALUE_NIL;
    }
    table->count = 0;
    for(int i = 0; i < table->capacity; i++) {
        if(table->entry[i].key == NULL) {
            continue;
        }
        Entry* dest = findEntry(new_entry, table->entry[i].key, capacity);
        dest->key = table->entry[i].key;
        dest->val = table->entry[i].val;
        table->count++;
    }

    if(table->entry) FREE(table->entry, "free entry\n");
    table->entry = new_entry;
	table->capacity = capacity;
}

bool tableSet(Table* table, ObjString* key, Value val) {
    if(table->count >= table->capacity * MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjustTable(table, capacity);
    }
    Entry* dest = findEntry(table->entry, key, table->capacity);
    // If find a tombstone, it's not a new item.
    bool is_new_key = IS_EMPTY_ENTRY(dest);
    if(is_new_key == true) {
        table->count++;
    }

    // Even there is a 'key' has same 'chars' with the 'new_key',
    // it doesn't matter.
    dest->key = key;
    dest->val = val;

    return is_new_key;
}

bool tableGet(Table* table, ObjString* key, Value* val) {
    if(table->count == 0) return false;
    Entry* entry = findEntry(table->entry, key, table->capacity);
    if(entry->key == NULL) return false;
    *val = entry->val;
    return true;
}

// Only set the entry a tombstone, not decrease the count.
bool tableDelete(Table* table, ObjString* key) {
    Entry* to_be_delete = findEntry(table->entry, key, table->capacity);
    if(to_be_delete->key == NULL) return false;

    // Treat a tombstone as an normal entry, so don't reduce 'table->count'.
    SET_TOMBSTONE(to_be_delete);
    return true;
}

ObjString* tableFindString(Table* table, const char* initial, int length, uint32_t hash) {
    if(table->count == 0) return NULL;

    int index = hash % table->capacity;
    Entry* tombstone = NULL;
    for(;;) {
        Entry* entry = &table->entry[index];

        if(IS_TOMBSTONE(entry)) {
            continue;
        } else if(IS_EMPTY_ENTRY(entry)) {
            return NULL;
        } else if(entry->key->length == length && entry->key->hash_code == hash \
                && strncmp(entry->key->chars, initial, length) == 0) {
            return entry->key;
        }

        index = (index + 1) % table->capacity;
    }
}
