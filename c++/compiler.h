#ifndef clox_compiler_h
#define clox_compiler_h

#include <string>
#include "object.h"
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

class Compiler;

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
  void error(const std::string &message);
  void errorAt(Token &token, const std::string &message);
  bool getHadError();
  void emitByte(uint8_t byte);
  void emitBytes(uint8_t byte1, uint8_t byte2);
  int emitJump(uint8_t instruction);
  void patchJump(int offset);
  void emitReturn();
  void emitConstant(Value value);
  void endCompiler();
  void number(bool canAssign);
  void string(bool canAssign);
  void grouping(bool canAssign);
  void unary(bool canAssign);
  void binary(bool canAssign);
  void parsePrecedence(Precedence precedence);
  void literal(bool canAssign);
  void declaration();
  void statement();
  bool match(TokenType type);
  bool check(TokenType type);
  void printStatement();
  void expressionStatement();
  void ifStatement();
  void synchronize();
  void varDeclaration();
  uint8_t parseVariable(const char *errorMessage);
  uint8_t identifierConstant(Token *name);
  void defineVariable(uint8_t global);
  void variable(bool canAssign);
  void namedVariable(Token name, bool canAssign);
  void block();
  void declareVariable();
  void addLocal(Token name);
  bool identifiersEqual(Token a, Token b);
  int resolveLocal(Compiler* compiler, Token* name);
  void markInitialized();
  void and_(bool canAssign);
  void or_(bool canAssign);
  void whileStatement();
  void emitLoop(int loopStart);
  void forStatement();
};

using ParseFn = void (Parser::*)(bool canAssign);

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

class Local
{
public:
  Token name;
  int depth;
  Local() {
    name = Token(TOKEN_ERROR, 0, 0, 0, "");
    depth = -1;
  }
};

class Compiler
{
private:
  Local locals[UINT8_COUNT];
  int localCount;
  int scopeDepth;
  Compiler(int localCount, int scopeDepth) {
    this->localCount = localCount;
    this->scopeDepth = scopeDepth;
  }
  static Compiler *compiler_;

public:
  Compiler(Compiler &other) = delete;
  void operator=(const Compiler &) = delete;
  static Compiler *GetInstance();
  ~Compiler() {}
  void beginScope();
  void endScope(Parser* parser);
  int getScopeDepth();
  int getLocalCount();
  void incLocalCount();
  void decLocalCount();
  Local* getLocals();
};

#endif