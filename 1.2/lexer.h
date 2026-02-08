#ifndef LEXER_H
#define LEXER_H

#include <stdbool.h>

// Типы токенов
typedef enum {
    TOKEN_VAR, TOKEN_CONST, TOKEN_FUNC, TOKEN_IF, TOKEN_ELSE, TOKEN_ELIF,
    TOKEN_WHILE, TOKEN_FOR, TOKEN_IN, TOKEN_RETURN, TOKEN_TRUE, TOKEN_FALSE,
    TOKEN_NULL, TOKEN_AND, TOKEN_OR, TOKEN_NOT, TOKEN_BREAK, TOKEN_CONTINUE,
    TOKEN_CONSOLE, TOKEN_INPUT, TOKEN_LEN, TOKEN_APPEND, TOKEN_POP,
    TOKEN_KEYS, TOKEN_VALUES, TOKEN_TYPE, TOKEN_INT, TOKEN_FLOAT_FUNC,
    TOKEN_STR, TOKEN_BOOL, TOKEN_ARRAY, TOKEN_DICT, TOKEN_RANGE,
    TOKEN_FROM, TOKEN_USING,
    TOKEN_IDENTIFIER, TOKEN_NUMBER_INT, TOKEN_NUMBER_FLOAT, TOKEN_STRING,
    TOKEN_OPERATOR, TOKEN_PUNCTUATION, TOKEN_NEWLINE, TOKEN_EOF, TOKEN_ERROR
} TokenType;

// Структура токена
typedef struct {
    TokenType type;
    char* value;
    long int_val;
    double float_val;
    int line;
    int column;
    int length;
} Token;

// Структура лексера
typedef struct Lexer Lexer;

// Функции лексера
Lexer* lexer_create(const char* source, const char* filename);
void lexer_destroy(Lexer* lexer);
Token lexer_next(Lexer* lexer);
Token lexer_current(Lexer* lexer);
Token lexer_peek_token(Lexer* lexer, int lookahead);
void lexer_skip(Lexer* lexer);
bool lexer_check(Lexer* lexer, TokenType type);
bool lexer_check_value(Lexer* lexer, TokenType type, const char* value);
bool lexer_expect(Lexer* lexer, TokenType type, const char* value, const char* error_msg);

// Утилиты
const char* token_type_name(TokenType type);
void token_print(const Token* token);
void test_lexer(void);

#endif