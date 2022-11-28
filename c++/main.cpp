#include "chunk.h"
#include <iostream>

int main(int argc, const char* argv[]) {
  Chunk chunk;

  int constant = chunk.addConstant(1.2);
  chunk.writeChunk(OP_CONSTANT);
  chunk.writeChunk(constant);

  chunk.writeChunk(OP_RETURN);
  chunk.disassembleChunk();
  chunk.freeChunk();
  return 0;
}