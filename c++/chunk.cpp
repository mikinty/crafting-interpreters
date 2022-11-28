#include "chunk.h"
#include <iomanip>
#include <iostream>
#include <fmt/core.h>
#include <fmt/printf.h>

void Chunk::writeChunk(uint8_t byte, int line) {
  this->code.push_back(byte);
  this->lines.push_back(line);
}

void Chunk::freeChunk() {
  this->code.clear();
}

// DEBUG: Printing instructions
void printValue(Value value) {
  fmt::printf("%g", value);
}

static int simpleInstruction(const std::string& name, int offset) {
  std::cout << name << "\n";
  return offset + 1;
}

int Chunk::constantInstruction(const std::string& name, int offset) {
  uint8_t constantIndex = this->code[offset + 1];
  fmt::printf("%-16s %4d '", name, constantIndex);
  printValue(this->constants[constantIndex]);
  std::cout << "'\n";
  return offset + 2;
}

int Chunk::disassembleInstruction(int offset) {
  std::cout << std::setfill('0') << std::setw(4) << offset << " ";
  if (offset > 0 && this->lines[offset] == this->lines[offset-1]) {
    std::cout << "   | ";
  } else {
    fmt::printf("%4d ", this->lines[offset]);
  }

  uint8_t instruction = this->code[offset];
  switch (instruction) {
    case OP_CONSTANT:
      return constantInstruction("OP_CONSTANT", offset);
      break;
    case OP_NEGATE:
      return simpleInstruction("OP_NEGATE", offset);
      break;
    case OP_ADD:
      return simpleInstruction("OP_ADD", offset);
      break;
    case OP_SUBTRACT:
      return simpleInstruction("OP_SUBTRACT", offset);
      break;
    case OP_MULTIPLY:
      return simpleInstruction("OP_MULTIPLY", offset);
      break;
    case OP_DIVIDE:
      return simpleInstruction("OP_DIVIDE", offset);
      break;
    case OP_RETURN:
      return simpleInstruction("OP_RETURN", offset);
      break;
    default:
      std::cout << fmt::format("Unknown opcode {}\n", instruction);
      return offset + 1;
  }
}

void Chunk::disassembleChunk() {
  for (int offset = 0; offset < this->code.size();) {
    offset = disassembleInstruction(offset);
  }
}

/**
 * @brief Adds a value to the constant pool
 * @return The index of the added value
 */
int Chunk::addConstant(Value value) {
  this->constants.push_back(value);
  return this->constants.size() - 1;
}