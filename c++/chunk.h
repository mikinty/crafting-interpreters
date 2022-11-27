#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include <vector>

enum OpCode {
  OP_RETURN,
};

class Chunk {
private:
  std::vector<uint8_t> code;
  int offset = 0;

public:
  void writeChunk(uint8_t byte);
  void disassembleChunk();
  void freeChunk();
};

#endif