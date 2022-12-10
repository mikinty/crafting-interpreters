#include "compiler.h"
#include "scanner.h"
#include "common.h"
#include <fstream>

void Parser::advance() {
  this->previous = this->current;

  for (;;) {
    this->current = this->scanner.scanToken();
    if (this->current.type != TOKEN_ERROR) break;

    this->errorAtCurrent(this->current.source);
  }
}

void Parser::errorAtCurrent(const std::string& message) {
  this->errorAt(this->previous, message);
}


void Parser::errorAt(Token& token, const std::string& message) {
  if (this->panicMode) return;
  this->panicMode = true;

  std::fprintf(stderr, "[line %d] Error", token.line);

  if (token.type == TOKEN_EOF) {
    std::fprintf(stderr, " at end");
  } else if (token.type == TOKEN_ERROR) {
    // Nothing
  } else {
    fprintf(stderr, " at '%.*s'", token.length, token.start);
  }

  fprintf(stderr, ": %s\n", message);
  this->hadError = true;
}

bool Parser::getHadError() {
  return this->hadError;
}

void Parser::consume(const TokenType type, const std::string& message) {
  if (this->current.type == type) {
    this->advance();
    return;
  }

  this->errorAtCurrent(message);
}

bool compile(std::string& source, Chunk& chunk) {
  Scanner scanner(source);
  auto current = Token(TOKEN_EOF, 0, 0, 0, source);
  auto previous = Token(TOKEN_EOF, 0, 0, 0, source);
  Parser parser(current, previous, scanner);
  parser.advance();
  parser.expression();
  parser.consume(TOKEN_EOF, "Expect end of expression");

  return !parser.getHadError();
}