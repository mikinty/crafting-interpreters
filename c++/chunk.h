#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include <vector>
#include <string>

enum OpCode {
  OP_CONSTANT,
  OP_NIL,
  OP_TRUE,
  OP_FALSE,
  OP_POP,
  OP_GET_GLOBAL,
  OP_DEFINE_GLOBAL,
  OP_SET_GLOBAL,
  OP_EQUAL,
  OP_GREATER,
  OP_LESS,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_NOT,
  OP_NEGATE,
  OP_PRINT,
  OP_RETURN,
};

typedef struct Obj Obj;
typedef struct ObjString ObjString;

typedef enum {
  VAL_BOOL,
  VAL_NIL,
  VAL_NUMBER,
  VAL_OBJ,
} ValueType;

typedef struct {
  ValueType type;
  union {
    bool boolean;
    double number;
    Obj* obj;
  } as;
} Value;

bool valuesEqual(Value a, Value b);

#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_OBJ(value) ((value).type == VAL_OBJ)

#define AS_OBJ(value) ((value).as.obj)
#define AS_BOOL(value) ((value).as.boolean)
#define AS_NUMBER(value) ((value).as.number)

#define BOOL_VAL(value) {VAL_BOOL, {.boolean = value}}
#define NIL_VAL {VAL_NIL, {.number=0}}
#define NUMBER_VAL(value) {VAL_NUMBER, {.number = value}}
#define OBJ_VAL(object) {VAL_OBJ, {.obj = (Obj*)object}}
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