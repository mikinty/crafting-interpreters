#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include <vector>
#include <string>

enum OpCode {
  OP_CONSTANT,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_NEGATE,
  OP_RETURN,
};

typedef enum {
  VAL_BOOL,
  VAL_NIL,
  VAL_NUMBER
} ValueType;

typedef struct {
  ValueType type;
  union {
    bool boolean;
    double number;
  } as;
} Value;

#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)

#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)

#define BOOL_VAL(value) {VAL_BOOL, {.boolean = value}}
#define NIL_VAL {VAL_NIL, {.number=0}}
#define NUMBER_VAL(value) {VAL_NUMBER, {.number = value}}
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
  std::vector<int>& getLines();
};

void printValue(Value value);

#endif