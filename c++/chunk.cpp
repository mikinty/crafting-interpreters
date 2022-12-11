#include "chunk.h"
#include <iomanip>
#include <iostream>
#include <fmt/core.h>
#include <fmt/printf.h>

void Chunk::writeChunk(uint8_t byte, int line) {
  code.push_back(byte);
  lines.push_back(line);
}

void Chunk::freeChunk() {
  code.clear();
}

// DEBUG: Printing instructions
void printValue(Value value) {
  switch (value.type) {
    case VAL_BOOL:
      fmt::printf(AS_BOOL(value) ? "true" : "false");
      break;
    case VAL_NIL:
      fmt::printf("nil");
      break;
    case VAL_NUMBER:
      fmt::printf("%g", AS_NUMBER(value));
      break;
  }
}

static int simpleInstruction(const std::string& name, int offset) {
  std::cout << name << "\n";
  return offset + 1;
}

int Chunk::constantInstruction(const std::string& name, int offset) {
  uint8_t constantIndex = code[offset + 1];
  fmt::printf("%-16s %4d '", name, constantIndex);
  printValue(constants[constantIndex]);
  std::cout << "'\n";
  return offset + 2;
}

int Chunk::disassembleInstruction(int offset) {
  std::cout << std::setfill('0') << std::setw(4) << offset << " ";
  if (offset > 0 && lines[offset] == lines[offset-1]) {
    std::cout << "   | ";
  } else {
    fmt::printf("%4d ", lines[offset]);
  }

  uint8_t instruction = code[offset];
  switch (instruction) {
    case OP_CONSTANT:
      return constantInstruction("OP_CONSTANT", offset);
    case OP_NIL:
      return simpleInstruction("OP_NIL", offset);
    case OP_TRUE:
      return simpleInstruction("OP_TRUE", offset);
    case OP_FALSE:
      return simpleInstruction("OP_FALSE", offset);
    case OP_NEGATE:
      return simpleInstruction("OP_NEGATE", offset);
    case OP_ADD:
      return simpleInstruction("OP_ADD", offset);
    case OP_SUBTRACT:
      return simpleInstruction("OP_SUBTRACT", offset);
    case OP_MULTIPLY:
      return simpleInstruction("OP_MULTIPLY", offset);
    case OP_DIVIDE:
      return simpleInstruction("OP_DIVIDE", offset);
    case OP_RETURN:
      return simpleInstruction("OP_RETURN", offset);
    default:
      std::cout << fmt::format("Unknown opcode {}\n", instruction);
      return offset + 1;
  }
}

void Chunk::disassembleChunk() {
  for (size_t offset = 0; offset < code.size();) {
    offset = disassembleInstruction(offset);
  }
}

/**
 * @brief Adds a value to the constant pool
 * @return The index of the added value
 */
int Chunk::addConstant(Value value) {
  constants.push_back(value);
  return constants.size() - 1;
}

std::vector<int>& Chunk::getLines() {
  return lines;
}