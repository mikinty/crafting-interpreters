#include "chunk.h"
#include <iomanip>
#include <iostream>
#include <fmt/core.h>

void Chunk::writeChunk(uint8_t byte) {
  this->code.push_back(byte);
}

void Chunk::freeChunk() {
  this->code.clear();
}

static int simpleInstruction(const std::string& name, int offset) {
  std::cout << name << "\n";
  return offset + 1;
}

void Chunk::disassembleChunk() {
  for (int offset = 0; offset < this->code.size(); offset++) {
    std::cout << std::setfill('0') << std::setw(4) << offset << " ";

    uint8_t instruction = this->code[offset];
    switch (instruction) {
      case OP_RETURN:
        offset = simpleInstruction("OP_RETURN", offset);
        break;
      default:
        std::cout << fmt::format("Unknown opcode {}\n", instruction);
        offset++;
    }
  }
}