#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "compiler.h"
#include "ram.h"
#include "scanner.h"
#include "value.h"
#include "object.h"
#include "vm.h"
#include "hint.h"

#define NUM_MAX 16
#define MAX_CLAUSE 30

typedef struct {
    Token previous;
    Token current;
    bool had_error;
} Parser;

typedef struct {
    Token local_variable_name;
    int scope_depth;
    bool need_capture;
} Local;

typedef struct {
    int index;
    bool is_local;
} UpValue;

// For each function.
typedef struct Compiler{
    ObjFunction* function;
    Local local[UINT8_MAX + 1];
    int local_count;
    UpValue upvalue[UINT8_MAX + 1];
    int scope_depth;
    struct Compiler* enclosing;
} Compiler;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)();
typedef struct {
    ParseFn prefix;
    ParseFn infix; 
    Precedence infixpre;
} ParseRule;

static Parser parser;
static Ram* current_ram = NULL;
static Compiler* current_stream = NULL;

static void beginCompile(Compiler* compiler, ObjString* func_name, FunctionType type) {
    compiler->function = allocateObjFunction(type);
    compiler->function->func_name = func_name;
    compiler->local_count = 0; 
    compiler->scope_depth = 0;
    compiler->enclosing = current_stream;
    current_stream = compiler;
    current_ram = &(current_stream->function->ram);
}

static void errorComile(const char* mes) {
    redHint("[error] at ");
    printf("[%d]: \'", parser.previous.line);
    for(int i = 0; i < parser.previous.length; i++) {
        printf("%c", parser.previous.initial[i]);
    }
    printf("\': ");
    redHint(mes);
    parser.had_error = true;
}

static void advance() {
    parser.previous = parser.current; 
    parser.current = scanToken();
    // printToken(&parser.current);
    if(parser.current.type == TOKEN_ERROR) {
        // redHint("Scan one error token.\n");
        errorComile("Scan one error token.\n");
    }
}

static void emitByte(uint8_t op_code) {
    writeCode(current_ram, op_code);
}

static void emitConstant(Value val) {
    writeConstant(current_ram, val);
}

static void consume(TokenType type, const char* mes) {
    TokenType token_type = parser.current.type;
    if(token_type != type) {
        // redHint(mes);
        errorComile(mes);
        return;
    }
    if(token_type != TOKEN_EOF) {
        advance();
    }
}

static void expression();
static ObjFunction* endCompile() {
    emitByte(OP_NIL);
    emitByte(OP_RETURN);

    ObjFunction* function = current_stream->function;
    current_stream = current_stream->enclosing;
    if(current_stream != NULL) {
        current_ram = &(current_stream->function->ram);
    }
    return function;
}

static void number();
static void unary();
static void binary();
static void group();
static void literal();
static void string();
static void variable();
static void and_();
static void or_();
static void call();
static ParseRule rules[] = {
    [TOKEN_IDENTIFIER]      = {variable,    NULL,       PREC_NONE},
    [TOKEN_NUMBER]          = {number,      NULL,       PREC_NONE},
    [TOKEN_STRING]          = {string,      NULL,       PREC_NONE},
    [TOKEN_NIL]             = {literal,     NULL,       PREC_NONE},
    [TOKEN_TRUE]            = {literal,     NULL,       PREC_NONE},
    [TOKEN_FALSE]           = {literal,     NULL,       PREC_NONE},
    [TOKEN_ADD]             = {NULL,        binary,     PREC_TERM},
    [TOKEN_SUBTRACT]        = {unary,       binary,     PREC_TERM},
    [TOKEN_MULTIPLY]        = {NULL,        binary,     PREC_FACTOR},
    [TOKEN_DIVIDE]          = {NULL,        binary,     PREC_FACTOR},
    [TOKEN_LEFT_PAREN]      = {group,       call,       PREC_CALL},
    [TOKEN_RIGHT_PAREN]     = {NULL,        NULL,       PREC_NONE},
    [TOKEN_BANG]            = {unary,       NULL,       PREC_NONE},
    [TOKEN_BANG_EQUAL]      = {NULL,        binary,     PREC_EQUALITY},
    [TOKEN_EQUAL_EQUAL]     = {NULL,        binary,     PREC_EQUALITY},
    [TOKEN_GREATER]         = {NULL,        binary,     PREC_COMPARISON},
    [TOKEN_LESS]            = {NULL,        binary,     PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL]   = {NULL,        binary,     PREC_COMPARISON},
    [TOKEN_LESS_EQUAL]      = {NULL,        binary,     PREC_COMPARISON},
    [TOKEN_EQUAL]           = {NULL,        NULL,       PREC_NONE},
    [TOKEN_LEFT_BRACE]      = {NULL,        NULL,       PREC_NONE},
    [TOKEN_RIGHT_BRACE]     = {NULL,        NULL,       PREC_NONE},
    [TOKEN_PRINT]           = {NULL,        NULL,       PREC_NONE},
    [TOKEN_VAR]             = {NULL,        NULL,       PREC_NONE},
    [TOKEN_IF]              = {NULL,        NULL,       PREC_NONE},
    [TOKEN_ELSE]            = {NULL,        NULL,       PREC_NONE},
    [TOKEN_ELIF]            = {NULL,        NULL,       PREC_NONE},
    [TOKEN_AND]             = {NULL,        and_,       PREC_AND},
    [TOKEN_OR]              = {NULL,        or_,        PREC_OR},
    [TOKEN_WHILE]           = {NULL,        NULL,       PREC_NONE},
    [TOKEN_FUNC]            = {NULL,        NULL,       PREC_NONE},
    [TOKEN_RETURN]          = {NULL,        NULL,       PREC_NONE},
    [TOKEN_SEMICOLON]       = {NULL,        NULL,       PREC_NONE},
    [TOKEN_ERROR]           = {NULL,        NULL,       PREC_NONE},
    [TOKEN_EOF]             = {NULL,        NULL,       PREC_NONE},
};

static ParseRule* getRule(TokenType type) {
    return &rules[type];
}

static void parsePrecedence(Precedence pre) {
    advance();
    ParseRule* rule = getRule(parser.previous.type);
    ParseFn prefix = rule->prefix;
    if(prefix == NULL) {
        errorComile("Expect expression.\n");
        return;
    }
    prefix();

    while(getRule(parser.current.type)->infixpre >= pre) {
        advance();
        ParseFn infix = getRule(parser.previous.type)->infix;
        infix();
    }
}

static void literal() {
    switch (parser.previous.type) {
        case TOKEN_FALSE:   emitByte(OP_FALSE); break;
        case TOKEN_NIL:     emitByte(OP_NIL); break;
        case TOKEN_TRUE:    emitByte(OP_TRUE); break;
        default: return; // Unreachable.
    }
}

static void number() {
    int length = parser.previous.length;
    if(length >= NUM_MAX) {
        // redHint("The length of the number is overflow.\n");
        errorComile("The length of the number is overflow.\n");
        return;
    }

    char num[NUM_MAX];
    memcpy(num, parser.previous.initial, length);
    num[length] = '\0';
    Value val = VALUE_NUMBER(atof(num));
    emitConstant(val);
}

static void unary() {
   TokenType type = parser.previous.type; 
    
    /* Parse something that has higher precedence than "type". */
    // TODO
    parsePrecedence(PREC_UNARY);
    /* Parse something that has higher precedence than "type". */
   
   switch(type) {
        case TOKEN_SUBTRACT: {
            emitByte(OP_NEGATE);
            break;
        }
        case TOKEN_BANG: {
            emitByte(OP_NOT);
            break;
        }
        default: {
            // redHint("Unknown Token.\n");
            errorComile("Unknown Token.\n");
            break;
        }
   }
}

static void binary() {
   TokenType type = parser.previous.type; 
    
    /* Parse something that has higher precedence than "type". */
    // TODO
    Precedence infix_pre = getRule(type)->infixpre;
    parsePrecedence(infix_pre + 1);
    /* Parse something that has higher precedence than "type". */
   
   switch(type) {
        case TOKEN_ADD: {
            emitByte(OP_ADD);
            break;
        }
        case TOKEN_SUBTRACT: {
            emitByte(OP_SUBTRACT);
            break;
        }
        case TOKEN_MULTIPLY: {
            emitByte(OP_MULTIPLY);
            break;
        }
        case TOKEN_DIVIDE: {
            emitByte(OP_DIVIDE);
            break;
        }
        case TOKEN_EQUAL_EQUAL: {
            emitByte(OP_EQUAL);
            break;
        }
        case TOKEN_BANG_EQUAL: {
            emitByte(OP_EQUAL);
            emitByte(OP_NOT);
            break;
        }
        case TOKEN_GREATER: {
            emitByte(OP_GREATER);
            break;
        }
        case TOKEN_LESS: {
            emitByte(OP_LESS);
            break;
        }
        case TOKEN_GREATER_EQUAL: {
            emitByte(OP_LESS);
            emitByte(OP_NOT);
            break;
        }
        case TOKEN_LESS_EQUAL: {
            emitByte(OP_GREATER);
            emitByte(OP_NOT);
            break;
        }
        default: {
            // redHint("Unknown Token.\n");
            errorComile("Unknown Token.\n");
            break;
        }
   }
}

static void string() {
    Token token = parser.previous;
    Value val = allocateString(token.initial, token.length);
    emitConstant(val);
}


static bool match(TokenType type) {
    if(parser.current.type == type) {
        advance();
        return true;
    }
    return false;
}

static void expression() {
    parsePrecedence(PREC_ASSIGNMENT);
}

static void emitJump(OpCode code);
static void and_() {
    emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    int begin_jump = current_ram->count;
    expression();
    int will_jump = current_ram->count - begin_jump + 1;
    current_ram->code[begin_jump - 3] = (uint8_t)will_jump;
    current_ram->code[begin_jump - 2] = (uint8_t)(will_jump >> 8);
}

static void or_() {
    emitJump(OP_JUMP_IF_FALSE);
    current_ram->code[current_ram->count - 2] = 3;
    current_ram->code[current_ram->count - 1] = 0x00;
    emitJump(OP_JUMP);
    emitByte(OP_POP);
    int begin_jump = current_ram->count;
    expression();
    int will_jump = current_ram->count - begin_jump + 1;
    current_ram->code[begin_jump - 3] = (uint8_t)will_jump;
    current_ram->code[begin_jump - 2] = (uint8_t)(will_jump >> 8);
}

static int getLocal(Token* token) {
    if(current_stream->scope_depth == 0) {
        return -1;
    }

    for(int i = current_stream->local_count - 1; i >= 0; i--) {
        Token* name = &current_stream->local[i].local_variable_name;
        if(current_stream->local[i].scope_depth <= current_stream->scope_depth \
                && strncmp(name->initial, token->initial, name->length) == 0) {
            if(current_stream->local[i].scope_depth == -1) {
                errorComile("Undefined local variable.\n");
            }
            return i;
        }
    }
    return -1;
}

static int addUpvalue(Compiler* stream, int index, bool is_local) {
    int* upvalue_count = &stream->function->upvalue_count;
    for(int i = 0; i < *upvalue_count; i++) {
        UpValue* tmp = &stream->upvalue[i];
        if(tmp->index == index && tmp->is_local == is_local) {
            return i;
        }
    }
    current_stream->upvalue[*upvalue_count].index = index;
    current_stream->upvalue[*upvalue_count].is_local = is_local;
    (*upvalue_count)++;
    return (*upvalue_count) - 1;
}

static int getUpValueFromEnclosing(Compiler* stream, Token* token) {
    if(stream == NULL) {
        return -1;
    }

    for(int i = 0; i < stream->local_count; i++) {
        Token* local_name = &stream->local[i].local_variable_name;
        if(local_name->length == token->length && strncmp(local_name->initial, token->initial, local_name->length) == 0) {
            stream->local[i].need_capture = true;
            return i;
        }
    }
    return -1;
}

static int getUpvalue(Compiler* stream, Token* token) {
    if(stream == NULL || stream->enclosing == NULL) return -1;

    int local = getUpValueFromEnclosing(stream->enclosing, token);
    if(local != -1) {
        return addUpvalue(stream, local, true);
    }

    int upvalue = getUpvalue(stream->enclosing, token);
    if(upvalue != -1) {
        return addUpvalue(stream, upvalue, false);
    }

    return -1;
}

static void variable() {
    uint8_t set_op;
    uint8_t get_op;
    int arg = 0;
    Token token = parser.previous;

    // Try to get a local variabl.
    arg = getLocal(&token);
    if(arg != -1) {
        set_op = OP_SET_LOCAL;
        get_op = OP_GET_LOCAL;
    } 
    else if((arg = getUpvalue(current_stream, &token)) != -1) {
        set_op = OP_SET_UPVALUE;
        get_op = OP_GET_UPVALUE;
    } 
    else {
        Value val = allocateString(token.initial, token.length);
        arg = addConstant(current_ram, val);
        set_op = OP_SET_GLOBAL;
        get_op = OP_GET_GLOBAL;
    }

    if(match(TOKEN_EQUAL)) {
        expression();
        emitByte(set_op);
    } else {
        emitByte(get_op);
    }
    emitByte(arg);
}

static void group() {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after group.\n");
}

static int scanParameters() {
    int arg_num = 0;
    if(parser.current.type != TOKEN_RIGHT_PAREN) {
        do {
            arg_num++;
            expression();
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after call.\n");
    return arg_num;
}

static void call() {
    int arg_num = scanParameters();
    emitByte(OP_CALL);
    emitByte(arg_num);
}

void justScan(const char* source) {
    initScanner(source);
    while(1) {
        Token token = scanToken();
        if(token.type == TOKEN_EOF) {
            break;
        }
        printToken(&token);
    }
}


static void printStmt() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.\n");
    emitByte(OP_PRINT);
}

static void expressionStmt() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.\n");
    emitByte(OP_POP);
}

static void addLocal() {
    if(current_stream->local_count == UINT8_MAX) {
        errorComile("The function local stack is overflow.\n");
    }
    Local* local = &current_stream->local[current_stream->local_count];
    local->local_variable_name = parser.previous;
    // current_stream->local[current_stream->local_count].scope_depth = current_stream->scope_depth;
    local->scope_depth = -1;
    local->need_capture = false;
    current_stream->local_count++;
}

static int resolveVariableName() {
    consume(TOKEN_IDENTIFIER, "Expect variable name.\n");

    // It's a local variable.
    if(current_stream->scope_depth > 0) {
        for(int i = current_stream->local_count - 1; i >= 0; i--) {
            Local* tmp = &current_stream->local[i];
            if(tmp->scope_depth == current_stream->scope_depth && \
                    tmp->local_variable_name.length == parser.previous.length && \
                    strncmp(tmp->local_variable_name.initial, parser.previous.initial, tmp->local_variable_name.length) == 0) {
                errorComile("The variable has existed.\n");
                return -1;
            }
        }
        addLocal();
        return -1;
    }

    // It's a global variable.
    return addConstant(current_ram, allocateString(parser.previous.initial, parser.previous.length));
}

static void markInit() {
    current_stream->local[current_stream->local_count - 1].scope_depth = current_stream->scope_depth;
}

static void varDeclaration() {
    int global_var_index = resolveVariableName();

    if(match(TOKEN_EQUAL)) {
        expression();
    } else {
        emitByte(OP_NIL);
    }
    consume(TOKEN_SEMICOLON, "Expect ';' after var statement.\n");
    
    // Mark the local variable initialized.
    if(global_var_index == -1) {
        markInit();
    } else {
        emitByte(OP_DEFINE_GLOBAL);
        emitByte((uint8_t)global_var_index);
    }
}

static void declaration();
static void blockStmt() {
    while(parser.current.type != TOKEN_RIGHT_BRACE && parser.current.type != TOKEN_EOF) {
        declaration();
    }
    consume(TOKEN_RIGHT_BRACE, "Expect '}', after block.\n");
}

static void endBlock() {
    for(int i = current_stream->local_count - 1; i >= 0; i--) {
        Local* local = &current_stream->local[i];
        if(local->scope_depth == current_stream->scope_depth) {
            emitByte(local->need_capture == true ? OP_CLOSE_UPVALUE : OP_POP);
            current_stream->local_count--;
        }
    }
    current_stream->scope_depth--;
}

static void emitJump(OpCode code) {
    emitByte(code);
    emitByte(0xff);
    emitByte(0xff);
}

static void statement();
static int ifClause() {
    consume(TOKEN_LEFT_PAREN, "Expect '(' after if or elif statement.\n");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after if or elif statement.\n");

    emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);

    int jump_begin = current_ram->count;
    statement();
    emitJump(OP_JUMP);  // TODO
    emitByte(OP_POP);
    int to_be_stuff = current_ram->count; 
    int jump_end = current_ram->count;
    uint16_t will_jump = jump_end - jump_begin;
    current_ram->code[jump_begin - 3] = (uint8_t)(will_jump);
    current_ram->code[jump_begin - 2] = (uint8_t)((will_jump >> 8));
    return to_be_stuff;
}

static void stuffJump(int clause_count, int* clause_to_do) {
    for(int i = 0; i < clause_count; i++) {
        int success_jump = current_ram->count - clause_to_do[i] + 1;
        current_ram->code[clause_to_do[i] - 3] = (uint8_t)success_jump;
        current_ram->code[clause_to_do[i] - 2] = (uint8_t)(success_jump >> 8);
    }
}

static void ifStmt() {
    int (*elifClause)() = ifClause;

    int clause_count = 0;
    int clause_to_do[MAX_CLAUSE];

    clause_to_do[clause_count] = ifClause();
    clause_count++;
    while(match(TOKEN_ELIF)) {
        clause_to_do[clause_count] = elifClause();
        clause_count++; 
    }
    
    // else clause.
    if(match(TOKEN_ELSE)) {
        statement();
    }

    stuffJump(clause_count, clause_to_do);
}

static void whileStmt() {
    int begin_back_jump = current_ram->count;
    consume(TOKEN_LEFT_PAREN, "Expect '(' after while statement.\n");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after while statement.\n");


    emitJump(OP_JUMP_IF_FALSE);
    int begin_jump = current_ram->count;
    statement();
    emitJump(OP_BACK_JUMP);
    emitByte(OP_POP);
    int end_jump = current_ram->count;
    int will_jump = end_jump - begin_jump - 1;
    int will_back_jump = end_jump - begin_back_jump - 1;
    current_ram->code[begin_jump - 2] = (uint8_t)will_jump;
    current_ram->code[begin_jump - 1] = (uint8_t)(will_jump >> 8);
    current_ram->code[end_jump - 3] = (uint8_t)will_back_jump;
    current_ram->code[end_jump - 2] = (uint8_t)(will_back_jump >> 8);
}

static int argList() {
    int arg_num = 0;
    consume(TOKEN_LEFT_PAREN, "Expect '(' before function parameters declaration.\n");
    if(parser.current.type != TOKEN_RIGHT_PAREN) {
        do {
            arg_num++;
            resolveVariableName();
            markInit();
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after function declaration.\n");
    return arg_num;
}

static void funcDeclaration() {
    int global_var_index = resolveVariableName();
    ObjString* func_name = allocateObjString(parser.previous.initial, parser.previous.length);

    Compiler compiler;
    beginCompile(&compiler, func_name, TYPE_USER);
    current_stream->scope_depth++;  // The function can't define a global variable.
    compiler.function->arity = argList();
    consume(TOKEN_LEFT_BRACE, "Expect '{' after function declaration.\n");
    blockStmt();
    ObjFunction* function = endCompile();

    emitByte(OP_CLOSURE);
    emitByte(addConstant(current_ram, VALUE_OBJ(function)));
    for(int i = 0; i < function->upvalue_count; i++) {
        emitByte(compiler.upvalue[i].is_local ? 1 : 0);
        emitByte(compiler.upvalue[i].index);
    }

    // Mark the local variable initialized.
    if(global_var_index == -1) {
        markInit();
    } else {
        emitByte(OP_DEFINE_GLOBAL);
        emitByte((uint8_t)global_var_index);
    }
}

static void returnStmt() {
    if(match(TOKEN_SEMICOLON)) {
        emitByte(OP_NIL);
        emitByte(OP_RETURN);
        return;
    } else {
        expression();
        emitByte(OP_RETURN);
    }
    consume(TOKEN_SEMICOLON, "Expect ';' after return statement.\n");
}

static void statement() {
    if(match(TOKEN_PRINT)) {
        printStmt();
    } else if(match(TOKEN_LEFT_BRACE)) {
        current_stream->scope_depth++;
        blockStmt();
        endBlock();
    } else if(match(TOKEN_IF)) {
        ifStmt();
    } else if(match(TOKEN_WHILE)) {
        whileStmt();
    } else if(match(TOKEN_RETURN)) {
        returnStmt();
    } else {
        expressionStmt();
    }
}

static void declaration() {
    if(match(TOKEN_VAR)) {
        varDeclaration();
    } else if(match(TOKEN_FUNC)) {
        funcDeclaration();
    } else {
        statement();
    }
}

ObjFunction* compile(const char* source) {
    initScanner(source);

    Compiler compiler;
    const char* main_func_name = "script";
    beginCompile(&compiler, allocateObjString(main_func_name, strlen(main_func_name)), TYPE_MAIN);
    advance(); 

    /* compile process */
    while(parser.current.type != TOKEN_EOF) {
        declaration();
    }
    /* compile process */

    consume(TOKEN_EOF, "Not find TOKEN_EOF\n");
    ObjFunction* main_func = endCompile();
    return parser.had_error == true ? NULL : main_func;
}

