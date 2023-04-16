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
  OP_GET_LOCAL,
  OP_SET_LOCAL,
  OP_DEFINE_GLOBAL,
  OP_GET_GLOBAL,
  OP_SET_GLOBAL,
  OP_GET_UPVALUE,
  OP_SET_UPVALUE,
  OP_EQUAL,
  OP_SET_PROPERTY,
  OP_GET_PROPERTY,
  OP_GET_SUPER,
  OP_GREATER,
  OP_LESS,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  OP_NOT,
  OP_NEGATE,
  OP_PRINT,
  OP_JUMP,
  OP_JUMP_IF_FALSE,
  OP_LOOP,
  OP_CALL,
  OP_INVOKE,
  OP_SUPER_INVOKE,
  OP_CLOSURE,
  OP_CLOSE_UPVALUE,
  OP_RETURN,
  OP_CLASS,
  OP_INHERIT,
  OP_METHOD,
};

typedef struct Obj Obj;
typedef struct ObjString ObjString;

#ifdef NAN_BOXING

#define SIGN_BIT ((uint64_t)0x8000000000000000)
#define QNAN     ((uint64_t)0x7ffc000000000000)

#define TAG_NIL   1
#define TAG_FALSE 2
#define TAG_TRUE  3

typedef uint64_t Value;

#define IS_BOOL(value) (((value) | 1) == TRUE_VAL)
#define IS_NIL(value) ((value) == NIL_VAL)
#define IS_NUMBER(value) (((value) & QNAN) != QNAN)
#define IS_OBJ(value) (((value) & (SIGN_BIT | QNAN)) == (QNAN | SIGN_BIT))

#define AS_BOOL(value)  ((value) == TRUE_VAL)
#define AS_NUMBER(value) valueToNum(value)
#define AS_OBJ(value) ((Obj*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))

#define BOOL_VAL(b) ((b) ? TRUE_VAL : FALSE_VAL)
#define FALSE_VAL   ((Value)(uint64_t)(QNAN | TAG_FALSE))
#define TRUE_VAL    ((Value)(uint64_t)(QNAN | TAG_TRUE))
#define NIL_VAL     ((Value)(uint64_t)(QNAN | TAG_NIL))
#define NUMBER_VAL(num) numToValue(num)
#define OBJ_VAL(obj) \
  (Value)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(obj))

static inline double valueToNum(Value value) {
  double num;
  memcpy(&num, &value, sizeof(Value));
  return num;
}

static inline Value numToValue(double num) {
  Value value;
  memcpy(&value, &num, sizeof(double));
  return value;
}

#else


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

#endif

bool valuesEqual(Value a, Value b);
class Chunk {
private:
  std::vector<int> lines;
  int offset = 0;
  int constantInstruction(const std::string& name, int offset);
  int invokeInstruction(const std::string& name, int offset);
  int byteInstruction(const std::string& name, int offset);
  int jumpInstruction(const std::string& name, int sign, int offset);

public:
  std::vector<Value> constants;
  std::vector<uint8_t> code;
  void writeChunk(uint8_t byte, int line);
  void disassembleChunk(const std::string& name);
  int disassembleInstruction(int offset);
  void freeChunk();
  int addConstant(Value value);
  std::vector<int>& getLines();
  int count();
};

void printValue(Value value);

#endif