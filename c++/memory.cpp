#include <stdlib.h>

#include "compiler.h"
#include "object.h"
#include "memory.h"
#include "vm.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "chunk.h"
#endif

void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
  if (newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
    collectGarbage();
#endif
  }

  if (newSize == 0) {
    free(pointer);
    return NULL;
  }

  void* result = realloc(pointer, newSize);
  if (result == NULL) exit(1);
  return result;
}

static void freeObject(Obj* object) {
#ifdef DEBUG_LOG_GC  
  printf("%p free type %d\n\n", (void*)object, object->type);
#endif
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

void collectGarbage() {
#ifdef DEBUG_LOG_GC  
  printf("-- gc begin\n");
#endif

  markRoots();


#ifdef DEBUG_LOG_GC  
  printf("-- gc end\n");
#endif
}

void markRoots() {
  auto vm = VM::GetInstance();
  for (size_t i = 0; i < vm->stack.size(); i++) {
    Value slot = vm->stack[i];
    markValue(slot);
  }

  for (int i = 0; i < vm->frameCount; i++) {
    markObject((Obj*)vm->frames[i].closure);
  }

  for (ObjUpvalue* upvalue = vm->openUpvalues; upvalue != NULL; upvalue = upvalue->next) {
    markObject((Obj*)upvalue);
  }

  // markTable
  markTable(vm->globals);
  markCompilerRoots();
}

void markTable(std::map<ObjString *, Value>& globals) {
  for (auto it = globals.begin(); it != globals.end(); ++it) {
    markObject((Obj*)it->first);
    markValue(it->second);
  }
}

void markValue(Value& value) {
  if (IS_OBJ(value)) markObject(AS_OBJ(value));
}

void markObject(Obj* object) {
  if (object == NULL) return;
#ifdef DEBUG_LOG_GC  
  printf("%p mark ", (void*)object);
  printValue(OBJ_VAL(object));
  printf("\n");
#endif
  object->isMarked = true;
}