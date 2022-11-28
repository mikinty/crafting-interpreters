#include "vm.h"
#include "chunk.h"
#include <iostream>

InterpretResult VM::run() {
  this->ip = 0;
  while (this->ip < this->chunk.code.size()) {
    uint8_t instruction = this->chunk.code[this->ip++];

    switch (instruction) {
      case OP_CONSTANT:
        {
          Value constant = this->chunk.constants[this->chunk.code[this->ip++]];
          printValue(constant);
          std::cout << "\n";
          break;
        }
      case OP_RETURN:
        return INTERPRET_OK;
    }
  }
}

InterpretResult VM::interpret(Chunk chunk) {
  this->chunk = chunk;
  return run();
}