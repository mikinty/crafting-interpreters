#include "vm.h"
#include "chunk.h"
#include "compiler.h"
#include <iostream>

InterpretResult VM::run() {
#define BINARY_OP(op) \
  do { \
    Value b = this->stack.back();\
    this->stack.pop_back();\
    this->stack.back() = this->stack.back() op b;\
  } while (false)

  this->ip = 0;
  while (this->ip < this->chunk.code.size()) {
    #ifdef DEBUG_TRACE_EXECUTION
    for (Value value : this->stack) {
      std::cout << "[ ";
      printValue(value);
      std::cout << " ]";
    }
    std::cout << "\n";
    this->chunk.disassembleInstruction(this->ip);
    #endif
    uint8_t instruction = this->chunk.code[this->ip++];

    switch (instruction) {
      case OP_CONSTANT:
        {
          Value constant = this->chunk.constants[this->chunk.code[this->ip++]];
          this->stack.push_back(constant);
          break;
        }
      case OP_NEGATE:
        {
          this->stack.back() = -this->stack.back();
          break;
        }
      case OP_ADD: BINARY_OP(+); break;
      case OP_SUBTRACT: BINARY_OP(-); break;
      case OP_MULTIPLY: BINARY_OP(*); break;
      case OP_DIVIDE: BINARY_OP(/); break;
      case OP_RETURN:
        {
          Value backValue = this->stack.back();
          this->stack.pop_back();
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

  this->chunk = chunk;
  this->ip = 0;

  return run();
}