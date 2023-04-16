#include "vm.h"
#include "chunk.h"
#include "compiler.h"
#include <iostream>
#include <cstdarg>
#include "object.h"
#include "memory.h"
#include <string>
#include <cstring>

static bool isFalsey(Value value) {
  return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

void VM::concatenate() {
  ObjString* b = AS_STRING(stack.back());
  ObjString* a = AS_STRING(stack.back());
  int length = a->length + b->length;
  char* chars = ALLOCATE(char, length + 1);
  memcpy(chars, a->chars, a->length);
  memcpy(chars + a->length, b->chars, b->length);
  chars[length] = '\0';

  // We shouldn't pop these strings yet, because they can be garbage-collected
  // in the allocate for chars
  stack.pop_back();
  stack.pop_back();
  ObjString* result = takeString(chars, length);
  stack.push_back(OBJ_VAL(result));
}

InterpretResult VM::run() {
  CallFrame frame = frames[frameCount-1];
#define READ_BYTE() (frame.ip++)
#define READ_CONSTANT() (frame.closure->function->chunk.constants[READ_BYTE()])
// TODO: no idea what this READ_SHORT is doing with the ip, it probably doesn't work. I tried to have it mask into a 16-bit int.
#define READ_SHORT() \
  (frame.ip += 2, \
  (uint16_t)((frame.closure->function->chunk.code[frame.ip-2] << 8) | frame.closure->function->chunk.code[frame.ip-1]))
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
  while (frame.ip < frame.closure->function->chunk.code.size()) {
    #ifdef DEBUG_TRACE_EXECUTION
    for (Value value : stack) {
      std::cout << "[ ";
      printValue(value);
      std::cout << " ]";
    }
    std::cout << "\n";
    frame.closure->function->chunk.disassembleInstruction(frame.ip);
    #endif
    uint8_t instruction = frame.closure->function->chunk.code[frame.ip++];

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
      case OP_GET_UPVALUE: {
        uint8_t slot = READ_BYTE();
        stack.push_back(frame.closure->upvalues[slot]->location[0]);
        break;
      }
      case OP_SET_UPVALUE: {
        uint8_t slot = READ_BYTE();
        frame.closure->upvalues[slot]->location[0] = peek(0);
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
      case OP_CLOSURE: {
        ObjFunction* function = AS_FUNCTION(READ_CONSTANT());
        ObjClosure* closure = newClosure(function);
        stack.push_back(OBJ_VAL(closure));
        for (int i = 0; i < closure->upvalueCount; i++) {
          uint8_t isLocal = READ_BYTE();
          uint8_t index = READ_BYTE();
          if (isLocal) {
            closure->upvalues[i] = captureUpvalue(std::vector<Value>(frame.slots.begin() + index, frame.slots.end()));
          } else {
            closure->upvalues[i] = frame.closure->upvalues[index];
          }
        }
        break;
      }
      case OP_CLOSE_UPVALUE:
        closeUpvalues();
        stack.pop_back();
        break;
      case OP_RETURN:
        {
          Value result = stack.back();
          stack.pop_back();
          closeUpvalues();
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
      case OP_CLASS: {
        stack.push_back(OBJ_VAL(newClass(READ_STRING())));
        break;
      }
      case OP_GET_PROPERTY: {
        if (!IS_INSTANCE(peek(0))) {
          runtimeError("Only instances have properties.");
          return INTERPRET_RUNTIME_ERROR;
        }

        ObjInstance* instance = AS_INSTANCE(peek(0));
        ObjString* name = READ_STRING();

        if (instance->fields.find(name) != instance->fields.end()) {
          stack.pop_back(); // instance
          stack.push_back(instance->fields[name]);
          break;
        }

        runtimeError("Undefined property '%s'.", name->chars);
        return INTERPRET_RUNTIME_ERROR;
      }
      case OP_SET_PROPERTY: {
        if (!IS_INSTANCE(peek(1))) {
          runtimeError("Only instances have properties.");
          return INTERPRET_RUNTIME_ERROR;
        }

        ObjInstance* instance = AS_INSTANCE(peek(1));
        instance->fields[READ_STRING()] = peek(0);
        Value value = stack.back();
        stack.pop_back();
        stack.pop_back();
        stack.push_back(value);
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
  ObjClosure* closure = newClosure(function);
  stack.pop_back();
  stack.push_back(OBJ_VAL(closure));
  call(closure, 0);

  return run();
}

Value VM::peek(int distance) {
  return stack[stack.size() - 1 - distance];
}

bool VM::call(ObjClosure* closure, int argCount) {
  if (argCount != closure->function->arity) {
    runtimeError("Expected %d arguments but god %d.", closure->function->arity, argCount);
  }

  if (frameCount == FRAMES_MAX) {
    runtimeError("Stack overflow");
    return false;
  }

  CallFrame frame = frames[frameCount++];
  frame.closure = closure;
  frame.ip = 0;
  frame.slots = std::vector<Value>(stack.end() - argCount - 1, stack.end());
  return true;
}

bool VM::callValue(Value callee, int argCount) {
  if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {
      case OBJ_CLASS: {
        ObjClass* klass = AS_CLASS(callee);
        stack[stack.size() - 1 - argCount - 1] = OBJ_VAL(newInstance(klass));
        return true;
      }
      case OBJ_CLOSURE: 
        return call(AS_CLOSURE(callee), argCount);
      case OBJ_NATIVE: {
        NativeFn native = AS_NATIVE(callee);
        std::vector<Value> newSlots = std::vector<Value>(stack.end() - argCount, stack.end());
        Value result = native(argCount, newSlots);
        stack = std::vector<Value>(stack.begin(), stack.end() - argCount - 1);
        stack.push_back(result);
        return true;
      }
      default:
        break;
    }
  }
  runtimeError("Can only callfunctions and classes.");
  return false;
}

ObjUpvalue* VM::captureUpvalue(std::vector<Value> local) {
  ObjUpvalue* prevUpvalue = NULL;
  ObjUpvalue* upvalue = openUpvalues;
  while (upvalue != NULL && &upvalue->location[0] > &local[0]) {
    prevUpvalue = upvalue;
    upvalue = upvalue->next;
  }

  if (upvalue != NULL && &upvalue->location[0] == &local[0]) {
    return upvalue;
  }

  ObjUpvalue* createdUpvalue = newUpvalue(local);

  if (prevUpvalue == NULL) {
    openUpvalues = createdUpvalue;
  } else {
    prevUpvalue->next = createdUpvalue;
  }

  return createdUpvalue;
}

/**
 * TODO: Almost certain this is incorrect too. I really should've just used
 * pointers, but really dead now.
 */
void VM::closeUpvalues() {
  while (openUpvalues != NULL && &openUpvalues->location[0] >= &stack.back()) {
    ObjUpvalue* upvalue = openUpvalues;
    upvalue->closed = upvalue->location[0];
    /** TODO: I don't understand this code, in the book they do 
     * upvalue->closed = *upvalue->location;
     * upvalue->location = &upvalue->closed;
     * 
     * Isn't that leaving the location the exactly value as before?
     */
    // upvalue->location = &upvalue->closed;
    openUpvalues = upvalue->next;
  }
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
  auto line = frame.closure->function->chunk.getLines()[instruction];
  std::fprintf(stderr, "[line %d] in script\n", line);

  for (int i = frameCount - 1; i >= 0; i--) {
    CallFrame frame = frames[i];
    ObjFunction* function = frame.closure->function;
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

void VM::defineNative(const char* name, NativeFn function) {
  stack.push_back(OBJ_VAL(copyString(name, (int)strlen(name))));
  stack.push_back(OBJ_VAL(newNative(function)));
  globals[AS_STRING(stack[0])] = stack[1];
  stack.pop_back();
  stack.pop_back();
}
