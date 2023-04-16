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
  ObjClosure* closure;
  size_t ip;
  std::vector<uint8_t> code;
  /**
   * Slots contains the local variable values. This can change depending on the
   * call stack, e.g. if we go into a function, our slots will shift to the end
   * of the slots from before. Hence why it's called "slots" since we only see
   * a slot of all value at any given time.
   */
  std::vector<Value> slots;
} CallFrame;

class VM
{
private:
  InterpretResult run();
  void concatenate();

  /**
   * The Singleton's constructor should always be private to prevent direct
   * construction calls with the `new` operator.
   */
  VM()
  {
    bytesAllocated = 0;
    nextGC = 1024 * 1024;
    initString = NULL; // prevent GC from trying to collect on initString
    initString = copyString("init", 4);
  }

  static VM *vm_;

public:
  // For garbage collection
  CallFrame frames[FRAMES_MAX];
  int frameCount;
  std::vector<Value> stack;
  Obj *objects;
  ObjUpvalue *openUpvalues;
  std::vector<Obj*> grayStack;
  size_t bytesAllocated;
  size_t nextGC;

  // String interning
  std::map<uint32_t, ObjString*> strings;
  std::map<ObjString*, Value> globals;
  ObjString* initString;

  InterpretResult interpret(std::string &source);
  Value peek(int distance);
  bool callValue(Value callee, int argCount);
  bool call(ObjClosure* function, int argCount);
  void runtimeError(const char *format, ...);
  void defineNative(const char* name, NativeFn function);
  ObjUpvalue* captureUpvalue(std::vector<Value> local);
  void closeUpvalues();
  void defineMethod(ObjString* name);
  bool bindMethod(ObjClass* klass, ObjString* name);
  bool invoke(ObjString* name, int argCount);
  bool invokeFromClass(ObjClass* klass, ObjString* name, int argCount);
  
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
    initString = NULL; // Allow the interned string to get collected
    freeObjects();
  }
};

#endif