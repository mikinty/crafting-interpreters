#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include <vector>
#include <string>

enum OpCode {
  OP_CONSTANT,
  OP_RETURN,
};

typedef double Value;
class Chunk {
private:
  std::vector<uint8_t> code;
  std::vector<Value> constants;
  int offset = 0;

  int constantInstruction(const std::string& name, int offset);

public:
  void writeChunk(uint8_t byte);
  void disassembleChunk();
  void freeChunk();
  int addConstant(Value value);
};

#endif