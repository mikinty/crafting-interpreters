#include "vm.h"
#include "chunk.h"
#include <iostream>

InterpretResult VM::run() {
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
          Value backValue = this->stack.back();
          this->stack.pop_back();
          this->stack.push_back(-backValue);
          break;
        }
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
}

InterpretResult VM::interpret(Chunk chunk) {
  this->chunk = chunk;
  return run();
}