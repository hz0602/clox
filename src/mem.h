#ifndef __MEM_H__
#define __MEM_H__

#define GROW_CAPACITY(a) ((a) == 0 ? 8 : 2 * (a))
#define GROW_ARRAY(arr, type, count, new_capacity) (type*)growArray(arr, sizeof(type), count, new_capacity)

void* growArray(void* array, int size, int count, int capacity);
void FREE(void* ptr, const char* message);

#endif // !__MEM_H__
