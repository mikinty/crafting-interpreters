package java_lox.com.lox;

import static java_lox.com.lox.TokenType.*;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

class Scanner {
    private final String source;
    private final List<Token> tokens = new ArrayList<Token>();    
    private int start = 0;
    private int current = 0;
    private int line= 1;

    private static final Map<String, TokenType> keywords;

    static {
        keywords = new HashMap<String, TokenType>();
        keywords.put("and", AND);
        keywords.put("class", CLASS);
        keywords.put("else", ELSE);
        keywords.put("false", FALSE);
        keywords.put("for", FOR);
        keywords.put("fun", FUN);
        keywords.put("if", IF);
        keywords.put("nil", NIL);
        keywords.put("or", OR);
        keywords.put("print", PRINT);
        keywords.put("return", RETURN);
        keywords.put("super", SUPER);
        keywords.put("this", THIS);
        keywords.put("true", TRUE);
        keywords.put("var", VAR);
        keywords.put("while", WHILE);
    }

    Scanner (String source) {
        this.source = source;
    }

    List<Token> scanTokens() {
        while (!isAtEnd()) {
            start = current;
            scanToken();
        }

        tokens.add(new Token(EOF, "", null, line));
        return tokens;
    }

    private boolean isAtEnd() {
        return current >= source.length();
    } 

    private void scanToken() {
        char c = advance();

        switch(c) {
            case '(': addToken(LEFT_PAREN); break;
            case ')': addToken(RIGHT_PAREN); break;
            case '{': addToken(LEFT_BRACE); break;
            case '}': addToken(RIGHT_BRACE); break;
            case ',': addToken(COMMA); break;
            case '.': addToken(DOT); break;
            case '+': addToken(PLUS); break;
            case '-': addToken(MINUS); break;
            case ';': addToken(SEMICOLON); break;
            case '*': addToken(STAR); break;
            // two-character lexemes
            case '!':
                addToken(match('=') ? BANG_EQUAL : BANG);
                break;
            case '=':
                addToken(match('=') ? EQUAL_EQUAL : EQUAL);
                break;
            case '<':
                addToken(match('=') ? LESS_EQUAL : LESS);
                break;
            case '>':
                addToken(match('=') ? GREATER_EQUAL : GREATER);
                break;
            // longer lexemes
            case '/':
                if(match('/')) {
                    while (peek() != '\n' && !isAtEnd()) advance();
                } else if (match('*')) {
                    // Keep going until we see closing
                    while (!(peek() == '*' && peekNext() == '/')) {
                        // If we reach the end before getting to closing condition, we throw an error
                        if (isAtEnd()) {
                            Lox.error(line, "Unclosed block comment");
                            return;
                        }

                        if (peek() == '\n') line++;
                        advance();
                    }
                   
                    // Advance on the "*/"
                    advance();
                    advance();
                } else {
                    addToken(SLASH);
                }
                break;
            // ignore whitespace
            case ' ':
            case '\r':
            case '\t':
                break;
            
            case '\n':
                line++;
                break;

            // String literals
            case '"':
                string();
                break;

            // Numbers. The book chooses to use isDigit() in the default case,
            // but seems like bad style to me. This is more explicit and clear
            // for how the Scanner works.
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                number();
                break;

            default:
                if (isAlpha(c)) {
                    identifier();
                } else {
                    Lox.error(line, "Unexpected character: '" + c + "'");
                }

                break;
        }
    }

    private char advance() {
        return source.charAt(current++);
    }

    private void addToken(TokenType type) {
        addToken(type, null);
    }

    private void addToken(TokenType type, Object literal) {
        String text = source.substring(start, current);
        tokens.add(new Token(type, text, literal, line));
    }

    /**
     * Basically a peek and match for the expected character
     * @param expected the char we expect to see at the current position in the file
     * @return if expected matches the char we are currently at
     */
    private boolean match(char expected) {
        if (isAtEnd()) return false;
        if (source.charAt(current) != expected) return false;

        current++;
        return true;
    }

    /**
     * Personally I'm not sure why we need a peek() and a match()
     * EDIT: it's because sometimes we don't exactly know what we want to
     * expect, e.g. a number (but we don't know which number)
     * @return the character at the current position, if we are at the end we return '\0'
     */
    private char peek() {
        if (isAtEnd()) return '\0';
        return source.charAt(current);
    }

    private char peekNext() {
        if (current + 1 >= source.length()) return '\0';
        return source.charAt(current + 1);
    }

    private void string() {
        while (peek() != '"' && !isAtEnd()) {
            if (peek() == '\n') line++;
            advance();
        }

        if (isAtEnd()) {
            Lox.error(line, "Unterminated string, reached end of file without seeing another closing quotation.");
            return;
        }

        // Book says to use advance(), I think match() is more appropriate
        if (!match('"')) {
            Lox.error(line, "Expected closing quotation.");
            return;
        }

        // Trim surrounding quotes
        String value = source.substring(start + 1, current - 1);
        addToken(STRING, value);
    }

    /**
     * Scan a number that potentially has a decimal point.
     * 
     * NOTE: We do not support trailing decimals, e.g. "1234.""
     */
    private void number() {
        boolean isStart = true;
        while (isDigit(peek()))  {
            // Guarantee that we have a leading 0 followed by decimal
            if (advance() == '0' && isStart) {
                if (isDigit(peek())) {
                    Lox.error(line, "Leading 0 must be followed by a decimal point");
                    return;
                }

                // Allow for decimal, or just "0"
                break;
            }

            isStart = false;
        }

        if (peek() == '.' && isDigit(peekNext())) {
            advance(); // Consume "."

            while (isDigit(peek())) advance();
        }

        addToken(NUMBER, Double.parseDouble(source.substring(start, current)));
    }

    private void identifier() {
        while (isAlphaNumeric(peek())) advance();

        String text = source.substring(start, current);
        TokenType type = keywords.get(text);

        if (type == null) type = IDENTIFIER;

        addToken(type);
    }

    private boolean isDigit(char c) {
        return c >= '0' && c <= '9';
    }

    private boolean isAlpha(char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c < 'Z') || c == '_';
    }

    private boolean isAlphaNumeric(char c) {
        return isAlpha(c) || isDigit(c);
    }
}
