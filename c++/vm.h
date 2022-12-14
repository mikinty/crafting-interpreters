#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "memory.h"
#include <vector>
#include <map>

typedef enum
{
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR,
} InterpretResult;

class VM
{
private:
  Chunk chunk;
  size_t ip;
  std::vector<Value> stack;
  InterpretResult run();
  void concatenate();

  /**
   * The Singleton's constructor should always be private to prevent direct
   * construction calls with the `new` operator.
   */
  VM()
  {
  }

  static VM *vm_;

public:
  // For garbage collection
  Obj *objects;

  // String interning
  std::map<uint32_t, ObjString*> strings;
  std::map<ObjString*, Value> globals;

  InterpretResult interpret(std::string &source);
  Value peek(int distance);
  void runtimeError(const char *format, ...);
  /**
   * Singletons should not be cloneable.
   */
  VM(VM &other) = delete;
  /**
   * Singletons should not be assignable.
   */
  void operator=(const VM &) = delete;
  /**
   * This is the static method that controls the access to the singleton
   * instance. On the first run, it creates a singleton object and places it
   * into the static field. On subsequent runs, it returns the client existing
   * object stored in the static field.
   */
  static VM *GetInstance();
  // Destructor
  ~VM()
  {
    freeObjects();
  }
};

#endif