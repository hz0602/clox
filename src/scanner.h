#ifndef __SCANNER_H__
#define __SCANNER_H__

typedef enum {
    TOKEN_ERROR,

    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_NIL,
    TOKEN_TRUE,
    TOKEN_FALSE,

    TOKEN_ADD,          // +
    TOKEN_SUBTRACT,     // -
    TOKEN_MULTIPLY,     // *
    TOKEN_DIVIDE,       // /
    TOKEN_LEFT_PAREN,   // (
    TOKEN_RIGHT_PAREN,  // )
    TOKEN_BANG,         // !
    TOKEN_BANG_EQUAL,   // !=
    TOKEN_EQUAL_EQUAL,  // ==
    TOKEN_GREATER,      // >
    TOKEN_LESS,         // <
    TOKEN_GREATER_EQUAL,// >=
    TOKEN_LESS_EQUAL,   // <=
    TOKEN_EQUAL,        // =
    TOKEN_LEFT_BRACE,   // {
    TOKEN_RIGHT_BRACE,  // }
    TOKEN_COMMA,        // ,

    TOKEN_PRINT,
    TOKEN_VAR,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_ELIF,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_WHILE,
    TOKEN_FUNC,
    TOKEN_RETURN,

    TOKEN_SEMICOLON,    // ;

    TOKEN_EOF,
} TokenType;

typedef struct {
    TokenType type;
    const char* initial;
    int length;
    int line;
} Token;

void initScanner(const char* source);
Token scanToken();
void printToken(Token* token);

#endif // !__SCANNER_H__
