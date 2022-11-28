#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include <vector>
#include <string>

enum OpCode {
  OP_CONSTANT,
  OP_NEGATE,
  OP_RETURN,
};

typedef double Value;
class Chunk {
private:
  std::vector<int> lines;
  int offset = 0;

  int constantInstruction(const std::string& name, int offset);

public:
  std::vector<Value> constants;
  std::vector<uint8_t> code;
  void writeChunk(uint8_t byte, int line);
  void disassembleChunk();
  int disassembleInstruction(int offset);
  void freeChunk();
  int addConstant(Value value);
};

void printValue(Value value);

#endif