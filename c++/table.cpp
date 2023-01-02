#include "table.h"
#include "object.h"
#include "memory.h"

#define TABLE_MAX_LOAD 0.75

HashTable::HashTable() {
  count = 0;
  capacity = 0;
  entries = NULL;
}

bool HashTable::add(ObjString* key, Value value) {
  if (count + 1 > capacity * TABLE_MAX_LOAD) {
    int capacity = GROW_CAPACITY(capacity);
    adjustCapacity(capacity);
  }
  Entry* entry = findEntry(key);
  bool isNewKey = entry->key == NULL;
  if (isNewKey && IS_NIL(entry->value)) {
    count++;
  }

  entry->key = key;
  entry->value = value;
  return isNewKey;
}

Entry* tombstone = NULL;

Entry* HashTable::findEntry(ObjString* key) {
  uint32_t index = key->hash % capacity;
  for (;;) {
    Entry* entry = &entries[index];
    if (entry->key == NULL) {
      if (IS_NIL(entry->value)) {
        return tombstone != NULL ? tombstone : entry;
      } else {
        if (tombstone == NULL) tombstone = entry;
      }
    } else if (entry->key == key) {
      return entry;
    }

    index = (index + 1) % capacity;
  }
}

void HashTable::adjustCapacity(int Capacity) {
  Entry* entries = ALLOCATE(Entry, capacity);
  for (int i = 0; i < capacity; i++) {
    entries[i].key = NULL;
    entries[i].value = NIL_VAL;
  }

  // Re-fill table, notice tombstones get discarded here
  count = 0;
  for (int i = 0; i < capacity; i++) {
    Entry *entry = &entries[i];
    if (entry->key == NULL) continue;

    Entry* dest = findEntry(entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    count++;
  }

  FREE_ARRAY(Entry, entries, capacity);
  this->entries = entries;
  this->capacity = capacity;
}

bool HashTable::lookup(ObjString* key, Value* value) {
  if (count == 0) return false;

  Entry* entry = findEntry(key);
  if (entry->key == NULL) return false;

  *value = entry->value;
  return true;
}

bool HashTable::deleteEntry(ObjString* key) {
  if (count == 0) return false;

  Entry* entry = findEntry(key);
  if (entry->key == NULL) return false;

  // Place tombstone
  entry->key = NULL;
  entry->value = BOOL_VAL(true);
  return true;
}