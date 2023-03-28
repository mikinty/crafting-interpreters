#include <stdlib.h>

#include "object.h"
#include "memory.h"
#include "vm.h"

void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
  if (newSize == 0) {
    free(pointer);
    return NULL;
  }

  void* result = realloc(pointer, newSize);
  if (result == NULL) exit(1);
  return result;
}

static void freeObject(Obj* object) {
  switch (object->type) {
    case OBJ_CLOSURE: {
      ObjClosure* closure = (ObjClosure*)object;
      // TODO: not sure I did this freeing right. Also I really shouldn't be
      // explicitly freeing things in this program
      for (int i = 0; i < closure->upvalueCount; i++) {
        FREE(ObjUpvalue, closure->upvalues[i]);
      }
      
      FREE(ObjClosure, object) ;
      break;
    }
    case OBJ_STRING: {
      ObjString* string = (ObjString*)object;
      FREE_ARRAY(char, string->chars, string->length + 1);
      FREE(ObjString, object);
      break;
    }
    case OBJ_UPVALUE:
      FREE(ObjUpvalue, object);
      break;
    case OBJ_NATIVE: {
      FREE(ObjNative, object);
      break;
    }
    case OBJ_FUNCTION: {
      ObjFunction* function = (ObjFunction*)object;
      function->chunk.freeChunk();
      FREE(ObjFunction, object);
      break;
    }
  }
}

/**
 * Garbage Collector for the VM, just walks the object linked list and frees
 * all of the objects
 */
void freeObjects() {
  auto vm = VM::GetInstance();
  Obj* object = vm->objects;

  while (object != NULL) {
    Obj* next = object->next;
    freeObject(object);
    object = next;
  }
}
