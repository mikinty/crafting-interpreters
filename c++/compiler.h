#ifndef clox_compiler_h
#define clox_compiler_h

#include <string>
#include "vm.h"
#include "scanner.h"
#include "chunk.h"

typedef enum
{
  PREC_NONE,
  PREC_ASSIGNMENT, // =
  PREC_OR,         // or
  PREC_AND,        // and
  PREC_EQUALITY,   // == !=
  PREC_COMPARISON, // < > <= >=
  PREC_TERM,       // + -
  PREC_FACTOR,     // * /
  PREC_UNARY,      // ! -
  PREC_CALL,       // . ()
  PREC_PRIMARY
} Precedence;

bool compile(std::string &source, Chunk &chunk);

class Parser
{
private:
  Token &current;
  Token &previous;
  Scanner &scanner;
  Chunk compilingChunk;
  bool hadError;
  bool panicMode;

public:
  Parser(Token &current, Token &previous, Scanner &scanner, Chunk &chunk) : current(current), previous(previous), scanner(scanner), compilingChunk(chunk), hadError(false), panicMode(false) {}
  Chunk &currentChunk();
  uint8_t makeConstant(Value value);
  void advance();
  void expression();
  void consume(const TokenType type, const std::string &message);
  void errorAtCurrent(const std::string &message);
  void error(const std::string& message);
  void errorAt(Token &token, const std::string &message);
  bool getHadError();
  void emitByte(uint8_t byte);
  void emitBytes(uint8_t byte1, uint8_t byte2);
  void emitReturn();
  void emitConstant(Value value);
  void endCompiler();
  void number();
  void grouping();
  void unary();
  void binary();
  void parsePrecedence(Precedence precedence);
};

using ParseFn = void (Parser::*)();

class ParseRule
{
private:
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;

public:
  ParseRule() : prefix(NULL), infix(NULL), precedence(PREC_NONE) {}
  ParseRule(ParseFn prefix, ParseFn infix, Precedence precedence) : prefix(prefix), infix(infix), precedence(precedence) {}
  Precedence getPrecedence();
  ParseFn getPrefix();
  ParseFn getInfix();
};

#endif