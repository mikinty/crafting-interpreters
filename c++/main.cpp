#include "chunk.h"
#include "vm.h"
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

VM *VM::vm_ = nullptr;

/**
 * Static methods should be defined outside the class.
 */
VM *VM::GetInstance()
{
  /**
   * This is a safer way to create an instance. instance = new Singleton is
   * dangerous in case two instance threads wants to access at the same time
   */
  if (vm_ == nullptr)
  {
    vm_ = new VM();
  }
  return vm_;
}

static void repl(VM* vm) {
  std::string line;

  for (;;) {
    if (!std::getline(std::cin, line)) {
      std::cout << std::endl;
      break;
    }
    vm->interpret(line);
  }
}

static std::string readFile(const std::string& path) {
  std::ifstream fileStream(path);
  std::stringstream buffer;
  buffer << fileStream.rdbuf();
  return buffer.str();
}

static void runFile(VM* vm, const std::string& path) {
  std::string source = readFile(path);
  InterpretResult result = vm->interpret(source);

  if (result == INTERPRET_COMPILE_ERROR) exit(65);
  if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

int main(int argc, const char* argv[]) {
  auto vm = VM::GetInstance();

  if (argc == 1) {
    repl(vm);
  } else if (argc == 2) {
    runFile(vm, argv[1]);
  } else {
    std::fprintf(stderr, "Usage: clox [path]\n");
    exit(64);
  }

  return 0;
}