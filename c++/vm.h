#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include <vector>

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR,
} InterpretResult;

class VM {
  private:
    Chunk chunk;
    size_t ip;
    std::vector<Value> stack;
    InterpretResult run();

  public:
    InterpretResult interpret(Chunk chunk);
};

#endif