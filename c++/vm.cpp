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
  CallFrame frame = frames[frameCount-1];
#define READ_BYTE() (frame.ip++)
#define READ_CONSTANT() (frame.function->chunk.constants[READ_BYTE()])
// TODO: no idea what this READ_SHORT is doing with the ip, it probably doesn't work. I tried to have it mask into a 16-bit int.
#define READ_SHORT() \
  (frame.ip += 2, \
  (uint16_t)((frame.function->chunk.code[frame.ip-2] << 8) | frame.function->chunk.code[frame.ip-1]))
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
  frame.ip = 0;
  while (frame.ip < frame.function->chunk.code.size()) {
    #ifdef DEBUG_TRACE_EXECUTION
    for (Value value : stack) {
      std::cout << "[ ";
      printValue(value);
      std::cout << " ]";
    }
    std::cout << "\n";
    frame.function->chunk.disassembleInstruction(frame.ip);
    #endif
    uint8_t instruction = frame.function->chunk.code[frame.ip++];

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
        stack.push_back(frame.slots[slot]);
        break;
      }
      case OP_SET_LOCAL: {
        uint8_t slot = READ_BYTE();
        frame.slots[slot] = peek(0);
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
        frame.ip += offset;
        break;
      }
      case OP_JUMP_IF_FALSE: {
        uint16_t offset = READ_SHORT();
        if (isFalsey(peek(0))) {
          frame.ip += offset;
        }
        break;
      }
      case OP_LOOP: {
        uint16_t offset = READ_SHORT();
        frame.ip -= offset;
        break;
      }
      case OP_CALL: {
        int argCount = READ_BYTE();
        if (!callValue(peek(argCount), argCount)) {
          return INTERPRET_RUNTIME_ERROR;
        }
        frame = frames[frameCount - 1];
        break;
      }
      case OP_RETURN:
        {
          Value result = stack.back();
          stack.pop_back();
          frameCount--;
          if (frameCount == 0) {
            stack.pop_back();
            return INTERPRET_OK;
          }

          stack = frame.slots;
          stack.push_back(result);
          frame = frames[frameCount-1];
          break;
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
  ObjFunction* function = compile(source, chunk);

  if (function == NULL) {
    return INTERPRET_COMPILE_ERROR;
  }

  stack.push_back(OBJ_VAL(function));
  call(function, 0);

  return run();
}

Value VM::peek(int distance) {
  return stack[stack.size() - 1 - distance];
}

bool VM::call(ObjFunction* function, int argCount) {
  if (argCount != function->arity) {
    runtimeError("Expected %d arguments but god %d.", function->arity, argCount);
  }

  if (frameCount == FRAMES_MAX) {
    runtimeError("Stack overflow");
    return false;
  }

  CallFrame frame = frames[frameCount++];
  frame.function = function;
  frame.ip = 0;
  frame.slots = std::vector<Value>(stack.end() - argCount - 1, stack.end());
  return true;
}

bool VM::callValue(Value callee, int argCount) {
  if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {
      case OBJ_FUNCTION:
        return call(AS_FUNCTION(callee), argCount);
      default:
        break;
    }
  }
  runtimeError("Can only callfunctions and classes.");
  return false;
}

void VM::runtimeError(const char* format, ...) {
  va_list args;  
  va_start(args, format);
  std::vfprintf(stderr, format, args);
  va_end(args);
  std::fputs("\n", stderr);

  CallFrame frame = frames[frameCount - 1];
  // Since our ip is relative to the chunk, I think we can just leave it as just ip
  auto instruction = frame.ip;
  auto line = frame.function->chunk.getLines()[instruction];
  std::fprintf(stderr, "[line %d] in script\n", line);

  for (int i = frameCount - 1; i >= 0; i--) {
    CallFrame frame = frames[i];
    ObjFunction* function = frame.function;
    auto instruction = frame.ip;
    fprintf(stderr, "[line %d] in ", function->chunk.getLines()[instruction]);
    if (function->name == NULL) {
      fprintf(stderr, "script\n");
    } else {
      fprintf(stderr, "%s()\n", function->name->chars);
    }
  }
  
  // TODO: I only added this here because I don't have resetStack() in my code
  frameCount = 0;
  // resetStack();
}