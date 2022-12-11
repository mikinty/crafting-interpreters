#ifndef clox_scanner_h
#define clox_scanner_h

#include <string>

typedef enum {
  // Single-character tokens.
  TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
  TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
  TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
  TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,
  // One or two character tokens.
  TOKEN_BANG, TOKEN_BANG_EQUAL,
  TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
  TOKEN_GREATER, TOKEN_GREATER_EQUAL,
  TOKEN_LESS, TOKEN_LESS_EQUAL,
  // Literals.
  TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,
  // Keywords.
  TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
  TOKEN_FOR, TOKEN_FUN, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
  TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
  TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE,

  TOKEN_ERROR, TOKEN_EOF
} TokenType;

class Token {
  public:
    TokenType type;
    size_t start;
    size_t length;
    size_t line;
    std::string source;
    Token(TokenType type, size_t start, size_t length, size_t line, const std::string& source) {
      this->type = type;
      this->start = start;
      this->length = length;
      this->line = line;
      this->source = source;
    }
};

class Scanner {
  private:
    std::string source;
    size_t start;
    size_t current;
    size_t line;
    bool isAtEnd();
    bool match(char expected);
    void skipWhitespace();
    char peek();
    char peekNext();
    Token string();
    Token number();
    Token identifier();
    TokenType identifierType();
    TokenType checkKeyword(size_t start, size_t length, const std::string& rest, TokenType type);

  public:
    Scanner(const std::string& source);
    char advance();
    Token scanToken();
    Token makeToken(TokenType type);
    Token errorToken(const std::string& message);
};

#endif