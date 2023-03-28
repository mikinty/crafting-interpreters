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

typedef enum
{
  TYPE_FUNCTION,
  TYPE_SCRIPT,
} FunctionType;

ObjFunction *compile(std::string &source, Chunk &chunk);

class Compiler;

/**
 * It feels that the book does not do a good job of separating the Parser from
 * the compiler. I found that later on, especially once we started implementing
 * functions, that the Parser is essentially intertwined with the Compiler, and
 * there's little independence between them. My only separation for the two is
 * the fact that the Compiler is a singleton and needs to be accessed
 * separately. But I've already exposed lots of Compiler fields.
 *
 * In retrospect, I think I would've combined the parser and compiler into one
 * class, but it does seem a bit weird to do that based on what we learn about
 * the compiler and parser early on. The book just treats a lot of the compiler
 * as a static global, which doesn't do a good job of encapsulating what it is
 * independent from.
 */
class Parser
{
private:
  Token &current;
  Token &previous;
  Scanner &scanner;
  // TODO: May need to deprecate this since we now just use the ObjFunction on
  // the compiler
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
  ObjFunction *endCompiler();
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
  int resolveLocal(Compiler *compiler, Token *name);
  void markInitialized();
  void and_(bool canAssign);
  void or_(bool canAssign);
  void whileStatement();
  void emitLoop(int loopStart);
  void forStatement();
  void funDeclaration();
  void function(FunctionType type);
  void call(bool canAssign);
  uint8_t argumentList();
  void returnStatement();
  int resolveUpvalue(Compiler *compiler, Token *name);
  int addUpvalue(Compiler *compiler, uint8_t index, bool isLocal);
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
  bool isCaptured;
  Local()
  {
    name = Token(TOKEN_ERROR, 0, 0, 0, "");
    depth = -1;
    isCaptured = false;
  }
};

class Upvalue
{
public:
  uint8_t index;
  bool isLocal;
};

class Compiler
{
private:
  ObjFunction *function;
  FunctionType type;

  Local locals[UINT8_COUNT];
  int localCount;
  Upvalue upvalues[UINT8_COUNT];
  int scopeDepth;
  Compiler(FunctionType type)
  {
    this->function = NULL;
    this->type = type;
    this->localCount = 0;
    this->scopeDepth = 0;
    this->function = newFunction();

    Local local = this->locals[this->localCount++];
    local.depth = 0;
    local.name = Token();
  }
  static Compiler *compiler_;

public:
  Compiler *enclosing;
  Compiler(Compiler &other) = delete;
  void operator=(const Compiler &) = delete;
  static Compiler *GetInstance(bool newInstance, FunctionType type, ObjString *functionName);
  static void popCompiler();
  ~Compiler() {}
  void beginScope();
  void endScope(Parser *parser);
  int getScopeDepth();
  int getLocalCount();
  void incLocalCount();
  void decLocalCount();
  Local *getLocals();
  ObjFunction *getFunction();
  FunctionType getType();
  Upvalue* getUpvalues();
};

#endif