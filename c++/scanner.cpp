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
  start = 0;
  current = 0;
  line = 1;
}

bool Scanner::isAtEnd() {
  return current >= source.size();
}

char Scanner::advance() {
  current++;
  return source[current-1];
}

bool Scanner::match(char expected) {
  if (isAtEnd()) return false;
  if (source[current] != expected) return false;
  current++;
  return true;
}

Token Scanner::makeToken(TokenType type) {
  return Token(type, start, current - start, line, source);
}

Token Scanner::errorToken(const std::string& message) {
  return Token(TOKEN_ERROR, 0, message.size(), line, message);
}

char Scanner::peek() {
  return source[current];
}

char Scanner::peekNext() {
  if (isAtEnd()) return '\0';
  return source[current+1];
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
        line++;
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
    if (peek() == '\n') line++;
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

TokenType Scanner::checkKeyword(size_t start, size_t length, const std::string& rest, TokenType type) {
  return (source.substr(start, length) == rest) ? type : TOKEN_IDENTIFIER;
}

TokenType Scanner::identifierType() {
  switch (source[start]) {
    case 'a': return checkKeyword(1, 2, "nd", TOKEN_AND);
    case 'c': return checkKeyword(1, 4, "lass", TOKEN_CLASS);
    case 'e': return checkKeyword(1, 3, "lse", TOKEN_ELSE);
    case 'f': 
      if (current - start > 1) {
        switch (source[start+1]) {
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
      if (current - start > 1) {
        switch (source[start+1]) {
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
  start = current;

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