#include "compiler.h"
#include "scanner.h"
#include "common.h"

void compile(std::string& source) {
  Scanner scanner(source);

  int line = -1;
  for (;;) {
    Token token = scanner.scanToken();
    if (token.line != line) {
      std::printf("%4d ", token.line);
      line = token.line;
    } else {
      std::printf("   | ");
    }
    std::printf("%2d '%s'\n", token.type, token.source.substr(token.start, token.length));

    if (token.type == TOKEN_EOF) break;
  }
}