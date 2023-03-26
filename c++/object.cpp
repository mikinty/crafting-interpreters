#include <string>
#include "object.h"
#include "memory.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, objectType) \
    (type*)allocateObject(sizeof(type), objectType)

static uint32_t hashString(const char* key, int length) {
  uint32_t hash = 2166136261u;
  for (int i = 0; i < length; i++) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619;
  }
  return hash;
}

static Obj* allocateObject(size_t size, ObjType type) {
  Obj* object = (Obj*)reallocate(NULL, 0, size);
  object->type = type;

  auto vm = VM::GetInstance();
  object->next = vm->objects;
  vm->objects = object;
  return object;
}

ObjFunction* newFunction() {
  ObjFunction* function = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);
  function->arity = 0;
  function->name = NULL;
  function->chunk = Chunk();
  return function;
}

ObjNative* newNative(NativeFn function) {
  ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
  native->function = function;
  return native;
}

#define ALLOCATE_OBJ(type, objectType) \
  (type*)allocateObject(sizeof(type), objectType)

static ObjString* allocateString(char* chars, int length, uint32_t hash) {
  ObjString* str = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  str->length = length;
  str->chars = chars;
  str->hash = hash;

  auto vm = VM::GetInstance();
  vm->strings.insert(std::pair<uint32_t, ObjString*>(hash, str));

  return str;
}

ObjString* copyString(const char* chars, int length) {
  uint32_t hash = hashString(chars, length);

  auto vm = VM::GetInstance();
  auto search = vm->strings.find(hash);
  if (search != vm->strings.end()) {
    return vm->strings[hash];
  }

  char* heapChars = ALLOCATE(char, length + 1);
  memcpy(heapChars, chars, length);
  heapChars[length] = '\0';
  return allocateString(heapChars, length, hash);
}

static void printFunction(ObjFunction* function) {
  if (function->name == NULL) {
    printf("<script>");
    return;
  }
  printf("<fn %s>", function->name->chars);
}

void printObject(Value value) {
  switch (OBJ_TYPE(value)) {
    case OBJ_STRING:
      printf("%s", AS_CSTRING(value));
      break;
    case OBJ_FUNCTION:
      printFunction(AS_FUNCTION(value));
      break;
    case OBJ_NATIVE:
      printf("<native fn>");
      break;
  }
}

ObjString* takeString(char* chars, int length) {
  uint32_t hash = hashString(chars, length);

  auto vm = VM::GetInstance();
  auto search = vm->strings.find(hash);
  if (search != vm->strings.end()) {
    FREE_ARRAY(char, chars, length + 1);
    return vm->strings[hash];
  }
  
  return allocateString(chars, length, hash);
}
