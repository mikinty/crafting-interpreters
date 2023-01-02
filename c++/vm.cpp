#include "vm.h"
#include "chunk.h"
#include "compiler.h"
#include <iostream>
#include <cstdarg>
#include "object.h"
#include "memory.h"
#include <string>

static bool isFalsey(Value value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

void VM::concatenate() {
  ObjString* b = AS_STRING(stack.back());
  stack.pop_back();
  ObjString* a = AS_STRING(stack.back());
  stack.pop_back();
  int length = a->length + b->length;
  char* chars = ALLOCATE(char, length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';

  ObjString* result = takeString(chars, length);
  stack.push_back(OBJ_VAL(result));
}

InterpretResult VM::run() {
#define BINARY_OP(valueType, op) \
  do { \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
      runtimeError("Operands must be numbers."); \
      return INTERPRET_RUNTIME_ERROR; \
    } \
    double b = AS_NUMBER(stack.back());\
    stack.pop_back();\
    stack.back() = valueType(AS_NUMBER(stack.back()) op b);\
  } while (false)

  ip = 0;
  while (ip < chunk.code.size()) {
    #ifdef DEBUG_TRACE_EXECUTION
    for (Value value : stack) {
      std::cout << "[ ";
      printValue(value);
      std::cout << " ]";
    }
    std::cout << "\n";
    chunk.disassembleInstruction(ip);
    #endif
    uint8_t instruction = chunk.code[ip++];

    switch (instruction) {
      case OP_CONSTANT:
        {
          Value constant = chunk.constants[chunk.code[ip++]];
          stack.push_back(constant);
          break;
        }
      case OP_NEGATE:
        if (!IS_NUMBER(peek(0))) {
          runtimeError("Operand must be a number.");
          return INTERPRET_RUNTIME_ERROR;
        }
        {
          auto backValue = stack.back();
          stack.pop_back();
          stack.push_back(NUMBER_VAL(-AS_NUMBER(backValue)));
        }
        break;
      case OP_ADD: {
        if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
          concatenate();
        } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
          BINARY_OP(NUMBER_VAL, +);
        } else {
          runtimeError("Operands must be two numbers or two strings.");
          return INTERPRET_RUNTIME_ERROR;
        }
        break;
      }      
      case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
      case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
      case OP_DIVIDE:   BINARY_OP(NUMBER_VAL, /); break;
      case OP_RETURN:
        {
          Value backValue = stack.back();
          stack.pop_back();
          printValue(backValue);
          std::cout << "\n";
          return INTERPRET_OK;
        }
      case OP_NIL:
        stack.push_back(NIL_VAL);
        break;        
      case OP_TRUE:
        stack.push_back(BOOL_VAL(true));
        break;        
      case OP_FALSE:
        stack.push_back(BOOL_VAL(false));
        break;
      case OP_NOT: {
        auto backValue = stack.back();
        stack.pop_back();
        stack.push_back(BOOL_VAL(isFalsey(backValue)));
        break;
      }
      case OP_EQUAL: {
        Value b = stack.back();
        stack.pop_back();
        Value a = stack.back();
        stack.pop_back();
        stack.push_back(BOOL_VAL(valuesEqual(a, b)));
        break;
      }
      case OP_GREATER: BINARY_OP(BOOL_VAL, >); break;
      case OP_LESS: BINARY_OP(BOOL_VAL, <); break;
      default:
        runtimeError("Unimplemented instruction in VM run()");
        return INTERPRET_RUNTIME_ERROR;
    }
  }
#undef BINARY_OP
  runtimeError("Unreachable code at the end of VM run()");
  return INTERPRET_RUNTIME_ERROR;
}

InterpretResult VM::interpret(std::string& source) {
  Chunk chunk;

  if (!compile(source, chunk)) {
    return INTERPRET_COMPILE_ERROR;
  }

  chunk = chunk;
  ip = 0;
  objects = NULL;

  return run();
}

Value VM::peek(int distance) {
  return stack[stack.size() - 1 - distance];
}

void VM::runtimeError(const char* format, ...) {
  va_list args;  
  va_start(args, format);
  std::vfprintf(stderr, format, args);
  va_end(args);
  std::fputs("\n", stderr);

  auto instruction = ip - chunk.code.size() - 1;
  auto line = chunk.getLines()[instruction];
  std::fprintf(stderr, "[line %d] in script\n", line);
  // resetStack();
}