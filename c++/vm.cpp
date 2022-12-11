#include "vm.h"
#include "chunk.h"
#include "compiler.h"
#include <iostream>
#include <cstdarg>

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
      case OP_ADD:      BINARY_OP(NUMBER_VAL, +); break;
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