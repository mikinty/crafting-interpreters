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
#define READ_BYTE() (chunk.code[ip++])
#define READ_CONSTANT() (chunk.constants[chunk.code[ip++]])
// TODO: no idea what this READ_SHORT is doing with the ip, it probably doesn't work. I tried to have it mask into a 16-bit int.
#define READ_SHORT() (ip += 2, (uint16_t)((chunk.code[ip-2] << 8) | chunk.code[ip-1]))
#define READ_STRING() AS_STRING(READ_CONSTANT())
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

  auto vm = VM::GetInstance();
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
          Value constant = READ_CONSTANT();
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
      case OP_POP: {
        stack.pop_back(); 
        break;
      }
      case OP_GET_LOCAL: {
        uint8_t slot = READ_BYTE();
        vm->stack.push_back(vm->stack[slot]);
        break;
      }
      case OP_SET_LOCAL: {
        uint8_t slot = READ_BYTE();
        vm->stack[slot] = peek(0);
        break;
      }
      case OP_GET_GLOBAL: {
        ObjString* name = READ_STRING();
        if (vm->globals.find(name) == vm->globals.end()) {
          runtimeError("Undefined variable '%s'.", name->chars);
          return INTERPRET_RUNTIME_ERROR;
        }

        stack.push_back(vm->globals[name]);
        break;
      }
      case OP_DEFINE_GLOBAL: {
        ObjString* name = READ_STRING();
        vm->globals[name] = peek(0);
      }
      case OP_SET_GLOBAL: {
        ObjString* name = READ_STRING();
        if (vm->globals.find(name) == vm->globals.end()) {
          runtimeError("Undefined variable '%s'.", name->chars);
          return INTERPRET_RUNTIME_ERROR;
        }
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
      case OP_PRINT: {
        printValue(stack.back());
        stack.pop_back();
        printf("\n");
        break;
      }
      case OP_JUMP: {
        uint16_t offset = READ_SHORT();
        ip += offset;
        break;
      }
      case OP_JUMP_IF_FALSE: {
        uint16_t offset = READ_SHORT();
        if (isFalsey(peek(0))) {
          ip += offset;
        }
        break;
      }
      case OP_RETURN:
        {
          // Exit interpreter
          return INTERPRET_OK;
        }
      default:
        runtimeError("Unimplemented instruction in VM run()");
        return INTERPRET_RUNTIME_ERROR;
    }
  }
#undef READ_CONSTANT
#undef READ_STRING
#undef READ_SHORT
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