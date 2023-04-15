#ifndef clox_memory_h
#define clox_memory_h

#include "common.h"
#include <map>

#define ALLOCATE(type, count) \
  (type *)reallocate(NULL, 0, sizeof(type) * (count))

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

#define FREE_ARRAY(type, pointer, oldCount) \
  reallocate(pointer, sizeof(type) * (oldCount), 0)

#define GROW_CAPACITY(capacity) \
  ((capacity) < 8 ? 8 : (capacity)*2)

#define GROW_ARRAY(type, pointer, oldCount, newCount)    \
  (type *)reallocate(pointer, sizeof(type) * (oldCount), \
                     sizeof(type) * (newCount))

void *reallocate(void *pointer, size_t oldSize, size_t newSize);
void markRoots();
void markValue(Value& value);
void markObject(Obj* object);
void markTable(std::map<ObjString *, Value>& globals);
void collectGarbage();
void freeObjects();
void traceReferences();
void removeWhiteStrings(std::map<uint32_t, ObjString*>& strings);
void sweep();
void blackenObject(Obj* object);

#endif