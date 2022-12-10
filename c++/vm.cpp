#include "vm.h"
#include "chunk.h"
#include "compiler.h"
#include <iostream>

InterpretResult VM::run() {
#define BINARY_OP(op) \
  do { \
    Value b = stack.back();\
    stack.pop_back();\
    stack.back() = stack.back() op b;\
  } while (false)

  ip = 0;
  while (ip < chunk.code.size()) {
    #ifdef DEBUG_TRACE_EXECUTION
    for (Value value : stack) {
      std::cout << "[ ";
      printValue(value);
      std::cout << " ]";
    }
    std::cout << "\n";
    chunk.disassembleInstruction(ip);
    #endif
    uint8_t instruction = chunk.code[ip++];

    switch (instruction) {
      case OP_CONSTANT:
        {
          Value constant = chunk.constants[chunk.code[ip++]];
          stack.push_back(constant);
          break;
        }
      case OP_NEGATE:
        {
          stack.back() = -stack.back();
          break;
        }
      case OP_ADD: BINARY_OP(+); break;
      case OP_SUBTRACT: BINARY_OP(-); break;
      case OP_MULTIPLY: BINARY_OP(*); break;
      case OP_DIVIDE: BINARY_OP(/); break;
      case OP_RETURN:
        {
          Value backValue = stack.back();
          stack.pop_back();
          printValue(backValue);
          std::cout << "\n";
          return INTERPRET_OK;
        }
    }
  }
#undef BINARY_OP
}

InterpretResult VM::interpret(std::string& source) {
  Chunk chunk;

  if (!compile(source, chunk)) {
    return INTERPRET_COMPILE_ERROR;
  }

  chunk = chunk;
  ip = 0;

  return run();
}