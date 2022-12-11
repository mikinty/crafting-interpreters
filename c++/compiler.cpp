#include "compiler.h"
#include "scanner.h"
#include "common.h"
#include <fstream>
#include <iostream>
#include <map>
#include <fmt/core.h>

std::map<TokenType, ParseRule> rules = {
  {TOKEN_LEFT_PAREN, ParseRule(&Parser::grouping, NULL, PREC_NONE)},
  {TOKEN_RIGHT_PAREN, ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_LEFT_BRACE, ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_RIGHT_BRACE, ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_COMMA, ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_DOT, ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_MINUS, ParseRule(&Parser::unary, &Parser::binary, PREC_TERM)},
  {TOKEN_PLUS, ParseRule(NULL, &Parser::binary, PREC_TERM)},
  {TOKEN_SEMICOLON, ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_SLASH, ParseRule(NULL, &Parser::binary, PREC_FACTOR)},
  {TOKEN_STAR, ParseRule(NULL, &Parser::binary, PREC_FACTOR)},
  {TOKEN_BANG, ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_BANG_EQUAL, ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_EQUAL, ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_EQUAL_EQUAL, ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_GREATER, ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_GREATER_EQUAL, ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_LESS, ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_LESS_EQUAL, ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_IDENTIFIER, ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_STRING, ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_NUMBER, ParseRule(&Parser::number, NULL, PREC_NONE)},
  {TOKEN_AND, ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_CLASS, ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_ELSE, ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_FALSE, ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_FOR, ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_FUN, ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_IF, ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_NIL, ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_OR, ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_PRINT, ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_RETURN, ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_SUPER, ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_THIS, ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_TRUE, ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_VAR, ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_WHILE, ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_ERROR, ParseRule(NULL, NULL, PREC_NONE)},
  {TOKEN_EOF, ParseRule(NULL, NULL, PREC_NONE)}
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

  std::fprintf(stderr, "[line %d] Error", token.line);

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

void Parser::number() {
  double value = std::stod(previous.source.substr(previous.start, previous.length));
  emitConstant(NUMBER_VAL(value));
}

void Parser::grouping() {
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

void Parser::unary() {
  TokenType operatorType = previous.type;

  // Compile the operand
  parsePrecedence(PREC_UNARY);

  // Emit the operator instruction
  switch(operatorType) {
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

void Parser::binary() {
  TokenType operatorType = previous.type;
  ParseRule rule = getRule(operatorType);
  parsePrecedence((Precedence)(rule.getPrecedence() + 1));

  switch (operatorType) {
    case TOKEN_PLUS:  emitByte(OP_ADD); break;
    case TOKEN_MINUS: emitByte(OP_SUBTRACT); break;
    case TOKEN_STAR:  emitByte(OP_MULTIPLY); break;
    case TOKEN_SLASH: emitByte(OP_DIVIDE); break;
    default:
      error("This should not be reachable in binary operator types");
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

  (this->*prefixRule)();

  while (precedence <= getRule(current.type).getPrecedence()) {
    advance();
    ParseFn infixRule = getRule(previous.type).getInfix();
    (this->*infixRule)();
  }
}

void Parser::expression() {
  parsePrecedence(PREC_ASSIGNMENT);
}

bool compile(std::string& source, Chunk& chunk) {
  Scanner scanner(source);
  auto current = Token(TOKEN_EOF, 0, 0, 0, source);
  auto previous = Token(TOKEN_EOF, 0, 0, 0, source);
  Parser parser(current, previous, scanner, chunk);
  parser.advance();
  parser.expression();
  parser.consume(TOKEN_EOF, "Expect end of expression");
  parser.endCompiler();

  return !parser.getHadError();
}