#ifndef clox_table_h
#define clox_table_h

#include "common.h"
#include "chunk.h"

typedef struct
{
  ObjString *key;
  Value value;
} Entry;

class HashTable
{
private:
  int count;
  int capacity;
  Entry *entries;

public:
  HashTable();
  bool add(ObjString* key, Value value);
  bool lookup(ObjString* key, Value* value);
  bool deleteEntry(ObjString* key);
  Entry* findEntry(ObjString* key);
  void adjustCapacity(int capacity);
};

#endif