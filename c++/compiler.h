#ifndef clox_compiler_h
#define clox_compiler_h

#include <string>
#include "vm.h"
#include "scanner.h"

bool compile(std::string& source, Chunk& chunk);

class Parser {
  private:
    Token& current;
    Token& previous;
    Scanner& scanner;
    bool hadError;
    bool panicMode;
  public:
    Parser(Token& current, Token& previous, Scanner& scanner) : current(current), previous(previous), scanner(scanner), hadError(false), panicMode(false) {}
    void advance();
    void expression();
    void consume(const TokenType type, const std::string& message);
    void errorAtCurrent(const std::string& message);
    void errorAt(Token& token, const std::string& message);
    bool getHadError();
};

#endif