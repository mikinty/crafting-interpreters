#include "compiler.h"
#include "scanner.h"
#include "common.h"
#include <fstream>
#include <iostream>
#include <map>
#include <fmt/core.h>
#include "object.h"

Compiler *Compiler::compiler_ = nullptr;

Compiler *Compiler::GetInstance()
{
  if (compiler_ == nullptr)
  {
    compiler_ = new Compiler(0, 0);
  }
  return compiler_;
}
void Compiler::beginScope() {
  scopeDepth++;
}

void Compiler::endScope(Parser* parser) {
  scopeDepth--;

  while (localCount > 0 && locals[localCount-1].depth > scopeDepth) {
    parser->emitByte(OP_POP);
    localCount--;
  }
}

int Compiler::getScopeDepth() {
  return scopeDepth;
}

int Compiler::getLocalCount() {
  return localCount;
}

void Compiler::incLocalCount() {
  localCount++;
}

void Compiler::decLocalCount() {
  localCount--;
}

Local* Compiler::getLocals() {
  return locals;
}

std::map<TokenType, ParseRule> rules = {
  {TOKEN_LEFT_PAREN,    ParseRule(&Parser::grouping, NULL, PREC_NONE)},
  {TOKEN_RIGHT_PAREN,   ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_LEFT_BRACE,    ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_RIGHT_BRACE,   ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_COMMA,         ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_DOT,           ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_MINUS,         ParseRule(&Parser::unary, &Parser::binary, PREC_TERM)},
  {TOKEN_PLUS,          ParseRule(NULL, &Parser::binary, PREC_TERM)},
  {TOKEN_SEMICOLON,     ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_SLASH,         ParseRule(NULL, &Parser::binary, PREC_FACTOR)},
  {TOKEN_STAR,          ParseRule(NULL, &Parser::binary, PREC_FACTOR)},
  {TOKEN_BANG,          ParseRule(&Parser::unary, NULL, PREC_NONE)},
  {TOKEN_BANG_EQUAL,    ParseRule(NULL, &Parser::binary, PREC_EQUALITY)},
  {TOKEN_EQUAL,         ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_EQUAL_EQUAL,   ParseRule(NULL, &Parser::binary, PREC_EQUALITY)},
  {TOKEN_GREATER,       ParseRule(NULL, &Parser::binary, PREC_COMPARISON)},
  {TOKEN_GREATER_EQUAL, ParseRule(NULL, &Parser::binary, PREC_COMPARISON)},
  {TOKEN_LESS,          ParseRule(NULL, &Parser::binary, PREC_COMPARISON)},
  {TOKEN_LESS_EQUAL,    ParseRule(NULL, &Parser::binary, PREC_COMPARISON)},
  {TOKEN_IDENTIFIER,    ParseRule(&Parser::variable, NULL, PREC_NONE)},
  {TOKEN_STRING,        ParseRule(&Parser::string, NULL, PREC_NONE)},
  {TOKEN_NUMBER,        ParseRule(&Parser::number, NULL, PREC_NONE)},
  {TOKEN_AND,           ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_CLASS,         ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_ELSE,          ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_FALSE,         ParseRule(&Parser::literal, NULL, PREC_NONE)},
  {TOKEN_FOR,           ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_FUN,           ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_IF,            ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_NIL,           ParseRule(&Parser::literal, NULL, PREC_NONE)},
  {TOKEN_OR,            ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_PRINT,         ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_RETURN,        ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_SUPER,         ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_THIS,          ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_TRUE,          ParseRule(&Parser::literal, NULL, PREC_NONE)},
  {TOKEN_VAR,           ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_WHILE,         ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_ERROR,         ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_EOF,           ParseRule(NULL, NULL, PREC_NONE)}
};

/*** ParseRule Implementation ***/
Precedence ParseRule::getPrecedence() {
  return precedence;
}

ParseFn ParseRule::getPrefix() {
  return prefix;  
}

ParseFn ParseRule::getInfix() {
  return infix; 
}

/*** Parser Implementation ***/
void Parser::advance() {
  previous = current;

  for (;;) {
    current = scanner.scanToken();
    if (current.type != TOKEN_ERROR) break;

    errorAtCurrent(current.source);
  }
}

void Parser::errorAtCurrent(const std::string& message) {
  errorAt(current, message);
}

void Parser::error(const std::string& message) {
  errorAt(previous, message);
}

void Parser::errorAt(Token& token, const std::string& message) {
  if (panicMode) return;
  panicMode = true;

  std::fprintf(stderr, "[line %zu] Error", token.line);

  if (token.type == TOKEN_EOF) {
    std::fprintf(stderr, " at end");
  } else if (token.type == TOKEN_ERROR) {
    // Nothing
  } else {
    fprintf(stderr, "%s", fmt::format(" at '{}'", token.source).c_str());
  }

  fprintf(stderr, ": %s\n", message.c_str());
  hadError = true;
}

bool Parser::getHadError() {
  return hadError;
}

void Parser::consume(const TokenType type, const std::string& message) {
  if (current.type == type) {
    advance();
    return;
  }

  errorAtCurrent(message);
}

void Parser::emitByte(uint8_t byte) {
  currentChunk().writeChunk(byte, previous.line);
}

void Parser::emitBytes(uint8_t byte1, uint8_t byte2) {
  emitByte(byte1);
  emitByte(byte2);
}

Chunk& Parser::currentChunk() {
  return compilingChunk; 
}

void Parser::emitReturn() {
  emitByte(OP_RETURN);
#ifdef DEBUG_PRINT_CODE
  if (hadError) {
    std::cout << "finished with errors\n";
  }
  currentChunk().disassembleChunk();
#endif
}

uint8_t Parser::makeConstant(Value value) {
  int constant = compilingChunk.addConstant(value);
  if (constant > UINT8_MAX) {
    error("Too many constants in one chunk.");
    return 0;
  }

  return (uint8_t)constant;
}

void Parser::emitConstant(Value value) {
  emitBytes(OP_CONSTANT, makeConstant(value));
}

void Parser::endCompiler() {
  emitReturn();
}

void Parser::number(bool canAssign) {
  double value = std::stod(previous.source.substr(previous.start, previous.length));
  emitConstant(NUMBER_VAL(value));
}

void Parser::string(bool canAssign) {
  emitConstant(OBJ_VAL(copyString(previous.source.substr(previous.start+1, previous.length-2).c_str(), previous.length - 2)));
}

void Parser::variable(bool canAssign) {
  namedVariable(previous, canAssign);
}

void Parser::namedVariable(Token name, bool canAssign) {
  uint8_t getOp, setOp;
  Compiler* compiler = Compiler::GetInstance();
  int arg = resolveLocal(compiler, &name);
  if (arg != -1) {
    getOp = OP_GET_LOCAL;
    setOp = OP_SET_LOCAL;
  } else {
    arg = identifierConstant(&name);
    getOp = OP_GET_GLOBAL;
    setOp = OP_SET_GLOBAL;
  }

  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitBytes(setOp, arg);
  } else {
    emitBytes(getOp, arg);
  }
}

void Parser::grouping(bool canAssign) {
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

void Parser::unary(bool canAssign) {
  TokenType operatorType = previous.type;

  // Compile the operand
  parsePrecedence(PREC_UNARY);

  // Emit the operator instruction
  switch(operatorType) {
    case TOKEN_BANG:
      emitByte(OP_NOT);
      break;
    case TOKEN_MINUS:
      emitByte(OP_NEGATE);
      break;
    default:
      error("This should not be reachable in unary operator types");
      return;
  }
}

static ParseRule& getRule(TokenType type) {
  return rules[type];
}

void Parser::binary(bool canAssign) {
  TokenType operatorType = previous.type;
  ParseRule rule = getRule(operatorType);
  parsePrecedence((Precedence)(rule.getPrecedence() + 1));

  switch (operatorType) {
    case TOKEN_BANG_EQUAL:    emitBytes(OP_EQUAL, OP_NOT); break;
    case TOKEN_EQUAL_EQUAL:   emitByte(OP_EQUAL); break;
    case TOKEN_GREATER:       emitByte(OP_GREATER); break;
    case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
    case TOKEN_LESS:          emitByte(OP_LESS); break;
    case TOKEN_LESS_EQUAL:    emitBytes(OP_GREATER, OP_NOT); break;
    case TOKEN_PLUS:          emitByte(OP_ADD); break;
    case TOKEN_MINUS:         emitByte(OP_SUBTRACT); break;
    case TOKEN_STAR:          emitByte(OP_MULTIPLY); break;
    case TOKEN_SLASH:         emitByte(OP_DIVIDE); break;
    default:
      error("This should not be reachable in binary operator types");
      return;
  }
}

void Parser::literal(bool canAssign) {
  switch (previous.type) {
    case TOKEN_FALSE: emitByte(OP_FALSE); break;
    case TOKEN_TRUE: emitByte(OP_TRUE); break;
    case TOKEN_NIL: emitByte(OP_NIL); break;
    default: 
      error("This should not be reachable in literal types");
      return;
  }
}

void Parser::parsePrecedence(Precedence precedence) {
  advance();
  ParseFn prefixRule = getRule(previous.type).getPrefix();
  if (prefixRule == NULL) {
    error("Expect expression.");
    return;
  }

  bool canAssign = precedence <= PREC_ASSIGNMENT;
  (this->*prefixRule)(canAssign);

  while (precedence <= getRule(current.type).getPrecedence()) {
    advance();
    ParseFn infixRule = getRule(previous.type).getInfix();
    (this->*infixRule)(canAssign);
  }

  if (canAssign && match(TOKEN_EQUAL)) {
    error("Invalid assignment target.");
  }
}

void Parser::expression() {
  parsePrecedence(PREC_ASSIGNMENT);
}

void Parser::block() {
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    declaration();
  }

  consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

void Parser::expressionStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after value.");
  emitByte(OP_POP);
}

void Parser::printStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after value.");
  emitByte(OP_PRINT);
}

void Parser::synchronize() {
  panicMode = false;

  while (current.type != TOKEN_EOF) {
    if (previous.type == TOKEN_SEMICOLON) return;
    switch (current.type) {
      case TOKEN_CLASS:
      case TOKEN_FUN:
      case TOKEN_VAR:
      case TOKEN_FOR:
      case TOKEN_IF:
      case TOKEN_WHILE:
      case TOKEN_PRINT:
      case TOKEN_RETURN:
        return;
      default:
        // Do nothing
        ;
    }

    advance();
  }
}

void Parser::varDeclaration() {
  uint8_t global = parseVariable("Expect variable name.");

  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    emitByte(OP_NIL);
  }

  consume(TOKEN_SEMICOLON, "Expect ';' after value.");
  defineVariable(global);
}

uint8_t Parser::parseVariable(const char* errorMessage) {
  consume(TOKEN_IDENTIFIER, errorMessage);

  Compiler* compiler = Compiler::GetInstance();
  declareVariable();
  if (compiler->getScopeDepth() > 0) return 0;

  return identifierConstant(&previous);
}

void Parser::markInitialized() {
  Compiler* compiler = Compiler::GetInstance();
  compiler->getLocals()[compiler->getLocalCount() - 1].depth = compiler->getScopeDepth();
}

uint8_t Parser::identifierConstant(Token *name) {
  return makeConstant(OBJ_VAL(copyString(name->source.c_str(), name->length)));
}

bool Parser::identifiersEqual(Token a, Token b) {
  if (a.length != b.length) return false;
  return a.source == b.source;
}

int Parser::resolveLocal(Compiler* compiler, Token* name) {
  for (int i = compiler->getLocalCount() - 1; i >=0; i--) {
    Local* local = &compiler->getLocals()[i];
    if (identifiersEqual(*name, local->name)) {
      if (local->depth == -1) {
        error("Can't read local variable in its own initializer.");
      }
      return i;
    }
  }

  return -1;
}

void Parser::declareVariable() {
  Compiler* compiler = Compiler::GetInstance();
  if (compiler->getScopeDepth() == 0) return;

  for (int i = compiler->getLocalCount() - 1; i >= 0; i--) {
    Local* local = &compiler->getLocals()[i];
    if (local->depth != -1 && local->depth < compiler->getScopeDepth()) {
      break;
    }

    if (identifiersEqual(previous, local->name)) {
      error(fmt::format("Already a variable with name '{}' in this scope.", previous.source));
    }
  }
  addLocal(previous);
}

void Parser::addLocal(Token name) {
  Compiler* compiler = Compiler::GetInstance();
  if (compiler->getLocalCount() == UINT8_COUNT) {
    error("Too many local variables in function.");
    return;
  }

  Local* local = &compiler->getLocals()[compiler->getLocalCount()];
  local->name = name;
  local->depth = -1;
}

void Parser::defineVariable(uint8_t global) {
  Compiler* compiler = Compiler::GetInstance();
  if (compiler->getScopeDepth() > 0) {
    markInitialized();
    return;
  }
  
  emitBytes(OP_DEFINE_GLOBAL, global);
}

void Parser::declaration() {
  if (match(TOKEN_VAR)) {
    varDeclaration();
  } else {
    statement();
  }

  if (panicMode) synchronize();
}

void Parser::statement() {
  if (match(TOKEN_PRINT)) {
    printStatement();
  } else if (match(TOKEN_LEFT_BRACE)) {
    Compiler* compiler = Compiler::GetInstance();
    compiler->beginScope();
    block();
    compiler->endScope(this);
  } else {
    expressionStatement();
  }
}

bool Parser::match(TokenType type) {
  if (!check(type)) return false;
  advance();
  return true;
}

bool Parser::check(TokenType type) {
  return current.type == type;
}

bool compile(std::string& source, Chunk& chunk) {
  Scanner scanner(source);
  auto current = Token(TOKEN_EOF, 0, 0, 0, source);
  auto previous = Token(TOKEN_EOF, 0, 0, 0, source);
  Parser parser(current, previous, scanner, chunk);
  parser.advance();

  while (!parser.match(TOKEN_EOF)) {
    parser.declaration();
  }

  parser.endCompiler();

  return !parser.getHadError();
}