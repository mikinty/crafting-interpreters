#include <stdlib.h>

#include "compiler.h"
#include "object.h"
#include "memory.h"
#include "vm.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "chunk.h"
#endif

#define GC_HEAP_GROW_FACTOR 2

void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
  if (newSize > oldSize) {
#ifdef DEBUG_STRESS_GC
    collectGarbage();
#endif

  auto vm = VM::GetInstance();
  if (vm->bytesAllocated > vm->nextGC)
    collectGarbage();
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
  auto vm = VM::GetInstance();
#ifdef DEBUG_LOG_GC  
  printf("-- gc begin\n");
  size_t before = vm->bytesAllocated;
#endif

  markRoots();
  traceReferences();
  removeWhiteStrings(vm->strings);
  sweep();

  vm->nextGC = vm->bytesAllocated * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC  
  printf("-- gc end\n");
  printf("   collected %zu bytes (from %zu to %zu) next at %zu\n", before - vm->bytesAllocated, before, vm->bytesAllocated, vm->nextGC);
#endif
}

void removeWhiteStrings(std::map<uint32_t, ObjString*>& strings) {
  for (auto it = strings.begin(); it != strings.end(); ++it) {
    if (it->second != NULL && it->second->obj.isMarked) {
      strings.erase(it);
    }
  }
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

void traceReferences() {
  auto vm = VM::GetInstance();
  while (vm->grayStack.size() > 0) {
    Obj* object = vm->grayStack.back();
    vm->grayStack.pop_back();
    blackenObject(object);
  }
}

void sweep() {
  auto vm = VM::GetInstance();

  Obj* previous = NULL;
  Obj* object = vm->objects;
  while (object != NULL) {
    if (object->isMarked) {
      object->isMarked = false;
      previous = object;
      object = object->next;
    } else {
      Obj* unreached = object;
      object = object->next;
      if (previous != NULL) {
        previous->next = object;
      } else {
        vm->objects = object;
      }

      freeObject(unreached);
    }
  }
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

void markArray(std::vector<Value>& constants) {
  for (Value& value : constants) {
    markValue(value);
  }
}

void blackenObject(Obj* object) {
#ifdef DEBUG_LOG_GC  
  printf("%p blacken ", (void*)object);
  printValue(OBJ_VAL(object));
  printf("\n");
#endif
  switch (object->type) {
    case OBJ_CLOSURE: {
      ObjClosure* closure = (ObjClosure*)object;
      markObject((Obj*)closure->function);
      for (ObjUpvalue* upvalue : closure->upvalues) {
        markObject((Obj*)upvalue);
      }
      break;
    }
    case OBJ_FUNCTION: {
      ObjFunction* function = (ObjFunction*)object;
      markObject((Obj*)function->name);
      markArray(function->chunk.constants);
      break;
    }
    case OBJ_UPVALUE:
      markValue(((ObjUpvalue*)object)->closed);
      break;
    case OBJ_NATIVE:
    case OBJ_STRING:
      break;
  }
}

void markObject(Obj* object) {
  if (object == NULL) return;
  if (object->isMarked) return; // Prevent cycles
#ifdef DEBUG_LOG_GC  
  printf("%p mark ", (void*)object);
  printValue(OBJ_VAL(object));
  printf("\n");
#endif
  object->isMarked = true;

  auto vm = VM::GetInstance();
  vm->grayStack.push_back(object);
}