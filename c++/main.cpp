#include "chunk.h"
#include <iostream>

int main(int argc, const char* argv[]) {
  Chunk chunk;
  chunk.writeChunk(OP_RETURN);
  chunk.disassembleChunk();
  chunk.freeChunk();
  return 0;
}