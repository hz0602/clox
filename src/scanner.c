#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "scanner.h"

typedef struct {
    const char* start;
    const char* current;
    int line;
} Scanner;

static Scanner scanner;

void printToken(Token* token) {
    if(token->type == TOKEN_EOF) return;

    printf("line number: %d\tcontent: \"", token->line);

    int count = 0;
    while(count < token->length) {
        printf("%c", *(token->initial + count));
        count++;
    }
    printf("\"\t\t\ttype: ");
    
    switch(token->type) {
        case TOKEN_NUMBER: {
            printf("TOKEN_NUMBER");
            break;
        }
        case TOKEN_IDENTIFIER: {
            printf("TOKEN_IDENTIFIER");
            break;
        }
        case TOKEN_STRING: {
            printf("TOKEN_STRING");
            break;
        }
        case TOKEN_NIL: {
            printf("TOKEN_NIL");
            break;
        }
        case TOKEN_TRUE: {
            printf("TOKEN_TRUE");
            break;
        }
        case TOKEN_FALSE: {
            printf("TOKEN_FALSE");
            break;
        }
        case TOKEN_ADD: {
            printf("TOKEN_ADD");
            break;
        }
        case TOKEN_SUBTRACT: {
            printf("TOKEN_SUBTRACT");
            break;
        }
        case TOKEN_MULTIPLY: {
            printf("TOKEN_MULTIPLY");
            break;
        }
        case TOKEN_DIVIDE: {
            printf("TOKEN_DIVIDE");
            break;
        }
        case TOKEN_LEFT_PAREN: {
            printf("TOKEN_LEFT_PAREN");
            break;
        }
        case TOKEN_RIGHT_PAREN: {
            printf("TOKEN_RIGHT_PAREN");
            break;
        }
        case TOKEN_BANG: {
            printf("TOKEN_BANG");
            break;
        }
        case TOKEN_BANG_EQUAL: {
            printf("TOKEN_BANG_EQUAL");
            break;
        }
        case TOKEN_EQUAL_EQUAL: {
            printf("TOKEN_EQUAL_EQUAL");
            break;
        }
        case TOKEN_GREATER: {
            printf("TOKEN_GREATER");
            break;
        }
        case TOKEN_LESS: {
            printf("TOKEN_LESS");
            break;
        }
        case TOKEN_GREATER_EQUAL: {
            printf("TOKEN_GREATER_EQUAL");
            break;
        }
        case TOKEN_LESS_EQUAL: {
            printf("TOKEN_LESS_EQUAL");
            break;
        }
        case TOKEN_EQUAL: {
            printf("TOKEN_EQUAL");
            break;
        }
        case TOKEN_LEFT_BRACE: {
            printf("TOKEN_LEFT_BRACE");
            break;
        }
        case TOKEN_RIGHT_BRACE: {
            printf("TOKEN_RIGHT_BRACE");
            break;
        }
        case TOKEN_COMMA: {
            printf("TOKEN_COMMA");
            break;
        }
        case TOKEN_PRINT: {
            printf("TOKEN_PRINT");
            break;
        }
        case TOKEN_VAR: {
            printf("TOKEN_VAR");
            break;
        }
        case TOKEN_IF: {
            printf("TOKEN_IF");
            break;
        }
        case TOKEN_ELSE: {
            printf("TOKEN_ELSE");
            break;
        }
        case TOKEN_ELIF: {
            printf("TOKEN_ELIF");
            break;
        }
        case TOKEN_AND: {
            printf("TOKEN_AND");
            break;
        }
        case TOKEN_OR: {
            printf("TOKEN_OR");
            break;
        }
        case TOKEN_WHILE: {
            printf("TOKEN_WHILE");
            break;
        }
        case TOKEN_FUNC: {
            printf("TOKEN_FUNC");
            break;
        }
        case TOKEN_RETURN: {
            printf("TOKEN_RETURN");
            break;
        }
        case TOKEN_SEMICOLON: {
            printf("TOKEN_SEMICOLON");
            break;
        }
        case TOKEN_EOF: {
            printf("TOKEN_EOF");
            break;
        }
        case TOKEN_ERROR: {
            printf("TOKEN_ERROR");
            break;
        }
    }
    printf("\n");
    
}

void initScanner(const char* source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

static int isAtEnd() {
   return *(scanner.start) == '\0';
}

static char advance() {
    scanner.start = scanner.current; 
    scanner.current++;
    return *scanner.start;
}

static Token makeToken(TokenType type) {
    Token token;
    token.type = type;
    token.initial = scanner.start;
    token.length = (scanner.current - scanner.start);
    token.line = scanner.line;
    return token;
}

static Token errorToken(const char* mes) {
    Token token = { TOKEN_ERROR, mes, strlen(mes), scanner.line };
    return token;
}

static void skipWhitespace() {
    for(;;) {
        char ch = *scanner.current;
        if(ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
            if(ch == '\n') {
                scanner.line++;
            }
            scanner.current++;
            continue;
        }
        break;
    }
}

static int isDigit(const char ch) {
    return (ch >= '0' && ch <= '9');
}

static int isLetter(const char ch) {
    return ((ch == '_') || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'));
}

// Something unreasonable.
static Token number() {
    if(*scanner.start != '0') {
        for(;;) {
            char ch = *scanner.current;
            if(!isDigit(ch)) {
                break;
            }
            scanner.current++;
        }
    }
    if(*(scanner.current) == '.') {
        if(isDigit(*(scanner.current + 1))) {
            scanner.current++;
            for(;;) {
                char ch = *(scanner.current);
                if(!isDigit(ch)) {
                    break;
                }
                scanner.current++;
            }
        }
    }
    return makeToken(TOKEN_NUMBER);
}

static Token variableName() {
    for(;;) {
        char ch = *scanner.current;
        if(!isLetter(ch) && !isDigit(ch)) {
            break;
        }
        scanner.current++;
    }
    return makeToken(TOKEN_IDENTIFIER);
}

bool checkKeyWord(int length, const char* word) {
    if(strncmp(scanner.current, word, length) == 0) {
        return true;
    }
    return false;
}

static Token identifier() {
    if(*scanner.start == '_') {
        return variableName();
    }
    char ch = *scanner.start;
    switch(ch) {
        case 'a': {
            if(checkKeyWord(2, "nd")) {
                scanner.current += 2;
                return makeToken(TOKEN_AND);
            }
            break;
        }
        case 'd': {
            if(checkKeyWord(2, "ef")) {
                scanner.current += 2;
                return makeToken(TOKEN_FUNC);
            }
            break;
        }
        case 'e': {
            if(checkKeyWord(3, "lse")) {
                scanner.current += 3;
                return makeToken(TOKEN_ELSE);
            } else if(checkKeyWord(3, "lif")) {
                scanner.current += 3;
                return makeToken(TOKEN_ELIF);
            }
            break;
        }
        case 'f': {
            if(checkKeyWord(4, "alse")) {
                scanner.current += 4;
                return makeToken(TOKEN_FALSE);
            }
            break;
        }
        case 'i': {
            if(checkKeyWord(1, "f")) {
                scanner.current += 1;
                return makeToken(TOKEN_IF);
            }
            break;
        }
        case 'n': {
            if(checkKeyWord(2, "il")) {
                scanner.current += 2;
                return makeToken(TOKEN_NIL);
            }
            break;
        }
        case 'o': {
            if(checkKeyWord(1, "r")) {
                scanner.current += 1;
                return makeToken(TOKEN_OR);
            }
            break;
        }
        case 'p': {
            if(checkKeyWord(4, "rint")) {
                scanner.current += 4;
                return makeToken(TOKEN_PRINT);
            }
            break;
        }
        case 'r': {
            if(checkKeyWord(5, "eturn")) {
                scanner.current += 5;
                return makeToken(TOKEN_RETURN);
            }
        }
        case 't': {
            if(checkKeyWord(3, "rue")) {
                scanner.current += 3;
                return makeToken(TOKEN_TRUE);
            }
            break;
        }
        case 'v': {
            if(checkKeyWord(2, "ar")) {
                scanner.current += 2;
                return makeToken(TOKEN_VAR);
            }
            break;
        }
        case 'w': {
            if(checkKeyWord(4, "hile")) {
                scanner.current += 4;
                return makeToken(TOKEN_WHILE);
            }
            break;
        }
    }
    // return errorToken("I think this has no possibility to happen.\n");
    return variableName();
}

static Token string() {
    while(*scanner.current != '\"') {
        if(*scanner.current == '\0' || *scanner.current == '\r' || *scanner.current == '\n') {
            Token token = errorToken("Unterminated string.");
            if(*scanner.current == '\r') scanner.current++;
            return token;
        }
        scanner.current++;
    }
    scanner.start++;
    Token token = makeToken(TOKEN_STRING);
    scanner.current++;
    return token;
}

Token scanToken() {
    skipWhitespace();
    char ch = advance();
    if(isAtEnd()) {
        return makeToken(TOKEN_EOF);
    }
    if(isDigit(ch)) {
        return number();
    }
    if(isLetter(ch)) {
        return identifier();
    }
    switch(ch) {
        case '\"': return string();
        case '+': return makeToken(TOKEN_ADD);
        case '-': return makeToken(TOKEN_SUBTRACT);
        case '*': return makeToken(TOKEN_MULTIPLY);
        case '/': return makeToken(TOKEN_DIVIDE);
        case '(': return makeToken(TOKEN_LEFT_PAREN);
        case ')': return makeToken(TOKEN_RIGHT_PAREN);
        case '{': return makeToken(TOKEN_LEFT_BRACE);
        case '}': return makeToken(TOKEN_RIGHT_BRACE);
        case ',': return makeToken(TOKEN_COMMA);
        case '!': {
            if(*scanner.current == '=') {
                scanner.current++;
                return makeToken(TOKEN_BANG_EQUAL);
            }
            return makeToken(TOKEN_BANG);
        }
        case '=': {
            if(*scanner.current == '=') {
                scanner.current++;
                return makeToken(TOKEN_EQUAL_EQUAL);
            } else {
                return makeToken(TOKEN_EQUAL);
            }
        }
        case '>': {
            if(*scanner.current == '=') {
                scanner.current++;
                return makeToken(TOKEN_GREATER_EQUAL);
            }
            return makeToken(TOKEN_GREATER);
        }
        case '<': {
            if(*scanner.current == '=') {
                scanner.current++;
                return makeToken(TOKEN_LESS_EQUAL);
            }
            return makeToken(TOKEN_LESS);
        }
        case ';': return makeToken(TOKEN_SEMICOLON);
        default: return makeToken(TOKEN_ERROR);
    }
}

