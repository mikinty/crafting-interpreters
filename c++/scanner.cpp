#include "scanner.h"

static bool isDigit(char c) {
  return c >= '0' && c <= '9';
}

static bool isAlpha(char c) {
  return (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
         c == '_';
}

Scanner::Scanner(const std::string& source) {
  this->source = std::move(source);
  this->start = 0;
  this->current = 0;
  this->line = 1;
}

bool Scanner::isAtEnd() {
  return current >= source.size();
}

char Scanner::advance() {
  this->current++;
  return this->source[this->current-1];
}

bool Scanner::match(char expected) {
  if (isAtEnd()) return false;
  if (this->source[this->current] != expected) return false;
  this->current++;
  return true;
}

Token Scanner::makeToken(TokenType type) {
  return Token(type, this->start, this->current - this->start, this->line, this->source);
}

Token Scanner::errorToken(const std::string& message) {
  return Token(TOKEN_ERROR, 0, message.size(), this->line, message);
}

char Scanner::peek() {
  return this->source[this->current];
}

char Scanner::peekNext() {
  if (isAtEnd()) return '\0';
  return this->source[this->current+1];
}

void Scanner::skipWhitespace() {
  for (;;) {
    char c = peek();
    switch (c) {
      case ' ':
      case '\r':
      case '\t':
        advance();
        break;
      case '\n':
        this->line++;
        advance();
        break;
      case '/':
        if (peekNext() == '/') {
          while (peek() != '\n' && !isAtEnd()) advance();
        } else {
          return;
        }
        break;
      default:
        return;
    }
  }
}

Token Scanner::string() {
  while (peek() != '"' && !isAtEnd()) {
    if (peek() == '\n') this->line++;
    advance();
  }

  if (isAtEnd()) return errorToken("Unterminated string.");

  advance(); // closing quote
  return makeToken(TOKEN_STRING);
}

Token Scanner::number() {
  while (isDigit(peek())) advance();

  // Handle decimal numbers
  if (peek() == '.' && isDigit(peekNext())) {
    advance();

    while (isDigit(peek())) advance();
  }

  return makeToken(TOKEN_NUMBER);
}

Token Scanner::identifier() {
  while (isAlpha(peek()) || isDigit(peek())) advance();
  return makeToken(identifierType());
}

TokenType Scanner::checkKeyword(int start, int length, const std::string& rest, TokenType type) {
  return (this->source.substr(start, length) == rest) ? type : TOKEN_IDENTIFIER;
}

TokenType Scanner::identifierType() {
  switch (this->source[this->start]) {
    case 'a': return checkKeyword(1, 2, "nd", TOKEN_AND);
    case 'c': return checkKeyword(1, 4, "lass", TOKEN_CLASS);
    case 'e': return checkKeyword(1, 3, "lse", TOKEN_ELSE);
    case 'f': 
      if (this->current - this->start > 1) {
        switch (this->source[this->start+1]) {
          case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
          case 'o': return checkKeyword(2, 2, "or", TOKEN_FOR);          
          case 'u': return checkKeyword(2, 2, "un", TOKEN_FUN);
        }
      }
      break;
    case 'i': return checkKeyword(1, 1, "f", TOKEN_IF);
    case 'n': return checkKeyword(1, 2, "il", TOKEN_NIL);
    case 'o': return checkKeyword(1, 1, "r", TOKEN_OR);
    case 'p': return checkKeyword(1, 4, "rint", TOKEN_PRINT);
    case 'r': return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
    case 's': return checkKeyword(1, 4, "uper", TOKEN_SUPER);
    case 't': 
      if (this->current - this->start > 1) {
        switch (this->source[this->start+1]) {
          case 'h': return checkKeyword(2, 2, "is", TOKEN_THIS);          
          case 'r': return checkKeyword(2, 3, "rue", TOKEN_TRUE);          
        }
      }
      break;
    case 'v': return checkKeyword(1, 2, "ar", TOKEN_VAR);
    case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);
  }

  return TOKEN_IDENTIFIER;
}

Token Scanner::scanToken() {
  skipWhitespace();
  this->start = this->current;

  if (isAtEnd()) return makeToken(TOKEN_EOF);

  char c = advance();
  if (isAlpha(c)) return identifier();
  if (isDigit(c)) return number();

  switch (c) {
    case '(': return makeToken(TOKEN_LEFT_PAREN);
    case ')': return makeToken(TOKEN_RIGHT_PAREN);
    case '{': return makeToken(TOKEN_LEFT_BRACE);
    case '}': return makeToken(TOKEN_RIGHT_BRACE);
    case ';': return makeToken(TOKEN_SEMICOLON);
    case ',': return makeToken(TOKEN_COMMA);
    case '.': return makeToken(TOKEN_DOT);
    case '-': return makeToken(TOKEN_MINUS);
    case '+': return makeToken(TOKEN_PLUS);
    case '/': return makeToken(TOKEN_SLASH);
    case '*': return makeToken(TOKEN_STAR);
    case '!': 
      return makeToken(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case '=':
      return makeToken(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '<':
      return makeToken(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
    case '>':
      return makeToken(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
    case '"': return string();
  }

  return errorToken("Unexpected character.");
}