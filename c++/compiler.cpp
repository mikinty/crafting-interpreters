#include "compiler.h"
#include "scanner.h"
#include "common.h"
#include <fstream>
#include <iostream>
#include <map>
#include <fmt/core.h>
#include "object.h"
#include "memory.h"

Compiler *Compiler::compiler_ = nullptr;

/**
 * So this is kinda weird. In the book they have a linked list of Compilers.
 * The issue I sort of have is I wanted to make Compilers global, so I made it
 * a singleton. But the static method itself used to only return the single
 * compiler. Now that we have multiple compilers, what can we do? Well I think
 * it's agnostic to the current compiler, all the static function says is hey,
 * I have a reference to some compiler, and it used to use null to say it
 * exists and not null if it exists.
 * 
 * I think what I can do is just modify this static method so that it manages
 * the linked list. In the get instance, we have an option to add to the linked
 * list, otherwise we just return whatever the current pointer is. Each
 * Compiler now has a reference to its previous enclosing compiler as well.
 * 
 * popCompiler will take care of "killing" the existing compiler and then 
 */
Compiler *Compiler::GetInstance(bool newInstance = false, FunctionType type = TYPE_SCRIPT, ObjString* functionName = NULL)
{
  if (compiler_ == nullptr || newInstance)
  {
    Compiler* old_compiler = compiler_;
    compiler_ = new Compiler(type);
    if (type != TYPE_SCRIPT) {
      compiler_->function->name = functionName;
    }

    // Point back to the old compiler
    compiler_->enclosing = old_compiler;
  }
  return compiler_;
}

/**
 * This is the best way I could figure out managing the linked list of
 * compilers I have
 */
void Compiler::popCompiler() {
  Compiler* old_compiler = compiler_;
  compiler_ = compiler_->enclosing;

  // TODO: This is not exception safe. If the program throws an exception, we
  // will have memory that is never cleaned up...o well this is a bad compiler
  // design.
  delete old_compiler;
}

void Compiler::beginScope() {
  scopeDepth++;
}

void Compiler::endScope(Parser* parser) {
  scopeDepth--;

  while (localCount > 0 && locals[localCount-1].depth > scopeDepth) {
    if (locals[localCount-1].isCaptured) {
      parser->emitByte(OP_CLOSE_UPVALUE);
    } else {
      parser->emitByte(OP_POP);
    }
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

ObjFunction* Compiler::getFunction() {
  return function;
}

FunctionType Compiler::getType() {
  return type;
}

Upvalue* Compiler::getUpvalues() {
  return upvalues;
}

std::map<TokenType, ParseRule> rules = {
  {TOKEN_LEFT_PAREN,    ParseRule(&Parser::grouping, &Parser::call, PREC_CALL)},
  {TOKEN_RIGHT_PAREN,   ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_LEFT_BRACE,    ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_RIGHT_BRACE,   ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_COMMA,         ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_DOT,           ParseRule(NULL, &Parser::dot, PREC_CALL)},
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
  {TOKEN_AND,           ParseRule(NULL, &Parser::and_, PREC_AND)},
  {TOKEN_CLASS,         ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_ELSE,          ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_FALSE,         ParseRule(&Parser::literal, NULL, PREC_NONE)},
  {TOKEN_FOR,           ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_FUN,           ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_IF,            ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_NIL,           ParseRule(&Parser::literal, NULL, PREC_NONE)},
  {TOKEN_OR,            ParseRule(NULL, &Parser::or_, PREC_OR)},
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

void Parser::emitLoop(int loopStart) {
  emitByte(OP_LOOP);

  auto offset = currentChunk().count() - loopStart + 2;
  if (offset > UINT16_MAX) error("Loop body too large.");

  emitByte((offset >> 8) & 0xff);
  emitByte(offset & 0xff);
}

int Parser::emitJump(uint8_t instruction) {
  emitByte(instruction);
  emitByte(0xff);
  emitByte(0xff);
  // TODO: is this right instead of count?
  return currentChunk().code.size() - 2;
}

Chunk& Parser::currentChunk() {
  Compiler* compiler = Compiler::GetInstance();
  return compiler->getFunction()->chunk;
}

void Parser::emitReturn() {
  emitByte(OP_NIL);
  emitByte(OP_RETURN);
}

uint8_t Parser::makeConstant(Value value) {
  int constant = currentChunk().addConstant(value);
  if (constant > UINT8_MAX) {
    error("Too many constants in one chunk.");
    return 0;
  }

  return (uint8_t)constant;
}

void Parser::emitConstant(Value value) {
  emitBytes(OP_CONSTANT, makeConstant(value));
}

void Parser::patchJump(int offset) {
  // -2 to adjust for the bytecode for the jump offset itself
  int jump = currentChunk().count() - offset - 2;

  if (jump > UINT16_MAX) {
    error("Too much code to jump over.");
  }

  currentChunk().code[offset] = (jump >> 8) & 0xff;
  currentChunk().code[offset + 1] = jump & 0xff;
}

ObjFunction* Parser::endCompiler() {
  emitReturn();
  Compiler* compiler = Compiler::GetInstance();
  ObjFunction* function = compiler->getFunction();
#ifdef DEBUG_PRINT_CODE
  if (hadError) {
    std::cout << "finished with errors\n";
  }
  currentChunk().disassembleChunk(function->name != NULL ? function->name->chars : "script");
#endif
  // We exit the scope of the previous compiler
  Compiler::popCompiler();
  return function;
}

void Parser::number(bool canAssign) {
  double value = std::stod(previous.source.substr(previous.start, previous.length));
  emitConstant(NUMBER_VAL(value));
}

void Parser::or_(bool canAssign) {
  auto elseJump = emitJump(OP_JUMP_IF_FALSE);
  auto endJump = emitJump(OP_JUMP);

  patchJump(elseJump);
  emitByte(OP_POP);

  parsePrecedence(PREC_OR);
  patchJump(endJump);
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
  } else if ((arg = resolveUpvalue(compiler, &name)) != -1) {
    getOp = OP_GET_UPVALUE;
    setOp = OP_SET_UPVALUE;
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

void Parser::and_(bool canAssign) {
  auto endJump = emitJump(OP_JUMP_IF_FALSE);

  emitByte(OP_POP);
  parsePrecedence(PREC_AND);

  patchJump(endJump);
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

void Parser::call(bool canAssign) {
  uint8_t argCount = argumentList();
  emitBytes(OP_CALL, argCount);
}

void Parser::dot(bool canAssign) {
  consume(TOKEN_IDENTIFIER, "Expect property name after '.'");
  uint8_t name = identifierConstant(&previous);

  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitBytes(OP_SET_PROPERTY, name);
  } else {
    emitBytes(OP_GET_PROPERTY, name);
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

void Parser::function(FunctionType type) {
  Compiler* compiler = Compiler::GetInstance(true, TYPE_FUNCTION, copyString(previous.source.substr(previous.start, previous.length).c_str(), previous.length));
  compiler->beginScope();

  consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      compiler->getFunction()->arity++;
      if (compiler->getFunction()->arity > 255) {
        errorAtCurrent("Functions can't have more than 255 parameters.");
      }
      uint8_t constant = parseVariable("Expect parameter name.");
      defineVariable(constant);
    } while (match(TOKEN_COMMA));
  }
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after function params.");
  consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
  block();

  ObjFunction* function = endCompiler();
  emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function)));

  for (int i = 0; i < function->upvalueCount; i++) {
    emitByte(compiler->getUpvalues()[i].isLocal ? 1 : 0);
    emitByte(compiler->getUpvalues()[i].index);
  }
}

void Parser::funDeclaration() {
  uint8_t global = parseVariable("Expect function name.");
  markInitialized();
  function(TYPE_FUNCTION);
  defineVariable(global);
}

void Parser::classDeclaration() {
  consume(TOKEN_IDENTIFIER, "Expect class name.");
  uint8_t nameConstant = identifierConstant(&previous);
  declareVariable();

  emitBytes(OP_CLASS, nameConstant);
  defineVariable(nameConstant);

  consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");
  consume(TOKEN_RIGHT_BRACE, "Expect '}' after class body.");
}

void Parser::expressionStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after value.");
  emitByte(OP_POP);
}

void Parser::forStatement() {
  Compiler* compiler = Compiler::GetInstance();
  compiler->beginScope();
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");

  if (match(TOKEN_SEMICOLON)) {
    // no initializer
  } else if (match(TOKEN_VAR)) {
    varDeclaration();
  } else {
    expressionStatement();
  }

  int loopStart = currentChunk().count();
  int exitJump = -1;
  if (!match(TOKEN_SEMICOLON)) {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");    

    // Jump out of the loop if the condition is false
    exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
  }

  if (!match(TOKEN_RIGHT_PAREN)) {
    auto bodyJump = emitJump(OP_JUMP);
    auto incrementStart = currentChunk().count();
    expression();
    emitByte(OP_POP);
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clause");

    emitLoop(loopStart);
    loopStart = incrementStart;
    patchJump(bodyJump);
  }

  statement();
  emitLoop(loopStart);

  if (exitJump != -1) {
    patchJump(exitJump);
    emitByte(OP_POP); // Pop off the condition
  }

  compiler->endScope(this);
}

void Parser::ifStatement() {
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after if condition.");

  int thenJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  statement();

  int elseJump = emitJump(OP_JUMP);

  patchJump(thenJump);

  emitByte(OP_POP);
  if (match(TOKEN_ELSE)) {
    statement();
  }

  patchJump(elseJump);
}

void Parser::printStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after value.");
  emitByte(OP_PRINT);
}

void Parser::returnStatement() {
  Compiler* compiler = Compiler::GetInstance();
  if (compiler->getType() == TYPE_SCRIPT) {
    error("Can't return from the top level");
  }

  if (match(TOKEN_SEMICOLON)) {
    emitReturn();
  } else {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after return value.");
    emitByte(OP_RETURN);
  }
}

void Parser::whileStatement() {
  int loopStart = currentChunk().count();
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
  expression(); 
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

  auto exitJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  statement();
  emitLoop(loopStart);

  patchJump(exitJump);
  emitByte(OP_POP);
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
  if (compiler->getScopeDepth() == 0) return;
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
  
int Parser::addUpvalue(Compiler* compiler, uint8_t index, bool isLocal) {
  int upvalueCount = compiler->getFunction()->upvalueCount;

  for (int i = 0; i < upvalueCount; i++) {
    Upvalue* upvalue = &compiler->getUpvalues()[i];
    if (upvalue->index == index && upvalue->isLocal == isLocal) {
      return i;
    }
  }

  if (upvalueCount == UINT8_COUNT) {
    error("Too many closure variables in function");
    return 0;
  }

  compiler->getUpvalues()[upvalueCount].isLocal = isLocal;
  compiler->getUpvalues()[upvalueCount].index = index;
  return compiler->getFunction()->upvalueCount++;
}

int Parser::resolveUpvalue(Compiler* compiler, Token* name) {
  if (compiler->enclosing == NULL) return -1;

  int local = resolveLocal(compiler->enclosing, name);
  if (local != -1) {
    compiler->enclosing->getLocals()[local].isCaptured = true;
    return addUpvalue(compiler, (uint8_t)local, true);
  }

  int upvalue = resolveUpvalue(compiler->enclosing, name);
  if (upvalue != -1) {
    return addUpvalue(compiler, (uint8_t)upvalue, false);
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
  local->isCaptured = false;
}

void Parser::defineVariable(uint8_t global) {
  Compiler* compiler = Compiler::GetInstance();
  if (compiler->getScopeDepth() > 0) {
    markInitialized();
    return;
  }
  
  emitBytes(OP_DEFINE_GLOBAL, global);
}

uint8_t Parser::argumentList() {
  uint8_t argCount = 0;
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      expression();
      argCount++;
    } while (match(TOKEN_COMMA));
  }

  consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
  return argCount;
}

void Parser::declaration() {
  if (match(TOKEN_CLASS)) {
    classDeclaration();
  } else if (match(TOKEN_FUN)) {
    funDeclaration();
  } else if (match(TOKEN_VAR)) {
    varDeclaration();
  } else {
    statement();
  }

  if (panicMode) synchronize();
}

void Parser::statement() {
  if (match(TOKEN_PRINT)) {
    printStatement();
  } else if (match(TOKEN_FOR)) {
    forStatement();
  } else if (match(TOKEN_IF)) {
    ifStatement();
  } else if (match(TOKEN_RETURN)) {
    returnStatement();
  } else if (match(TOKEN_WHILE)) {
    whileStatement();
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

ObjFunction* compile(std::string& source, Chunk& chunk) {
  Scanner scanner(source);
  auto current = Token(TOKEN_EOF, 0, 0, 0, source);
  auto previous = Token(TOKEN_EOF, 0, 0, 0, source);
  Parser parser(current, previous, scanner, chunk);
  parser.advance();

  while (!parser.match(TOKEN_EOF)) {
    parser.declaration();
  }

  parser.endCompiler();

  ObjFunction* function = parser.endCompiler();
  return parser.getHadError() ? NULL : function;
}

void markCompilerRoots() {
  Compiler* compiler = Compiler::GetInstance();
  while (compiler != NULL) {
    markObject((Obj*)compiler->getFunction());
    compiler = compiler->enclosing;
  }
}