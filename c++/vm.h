#ifndef clox_vm_h
#define clox_vm_h

#include "object.h"
#include "chunk.h"
#include "memory.h"
#include <vector>
#include <map>

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef enum
{
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR,
} InterpretResult;

/**
 * We differ slightly from the book implementation here, where we have a
 * separate instruction pointer and code array. The book just uses the ip as a
 * pointer inside the code array, which I found to be weird, as it overloads
 * the definition of a pointer to be index and accessor.
 */
typedef struct {
  ObjFunction* function;
  size_t ip;
  std::vector<uint8_t> code;
  std::vector<Value> slots;
} CallFrame;

class VM
{
private:
  CallFrame frames[FRAMES_MAX];
  int frameCount;

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