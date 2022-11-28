#include "chunk.h"
#include "vm.h"
#include <iostream>

int main(int argc, const char* argv[]) {
  VM vm;
  Chunk chunk;

  int constant = chunk.addConstant(1.2);
  int lineNumber = 123;
  chunk.writeChunk(OP_CONSTANT, lineNumber);
  chunk.writeChunk(constant, lineNumber);

  constant = chunk.addConstant(3.4);
  chunk.writeChunk(OP_CONSTANT, lineNumber);
  chunk.writeChunk(constant, lineNumber);
  chunk.writeChunk(OP_ADD, lineNumber);

  constant = chunk.addConstant(5.6);
  chunk.writeChunk(OP_CONSTANT, lineNumber);
  chunk.writeChunk(constant, lineNumber);
  chunk.writeChunk(OP_DIVIDE, lineNumber);

  chunk.writeChunk(OP_NEGATE, lineNumber);
  chunk.writeChunk(OP_RETURN, lineNumber);
  chunk.disassembleChunk();
  vm.interpret(chunk);
  chunk.freeChunk();
  return 0;
}