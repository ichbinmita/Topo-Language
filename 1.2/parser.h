#ifndef PARSER_H
#define PARSER_H

#include "ast.h"

// Парсинг целой программы
ASTNode* parse_program(Parser* parser);

// Парсинг выражений (объявим для использования в parser.c)
ASTNode* parse_expression(Parser* parser);
ASTNode* parse_block(Parser* parser);
ASTNode* parse_statement(Parser* parser);

// Создание парсера
Parser* parser_create(Lexer* lexer);
void parser_destroy(Parser* parser);

// Основная функция парсинга
ASTNode* parse_source(const char* source, const char* filename);

#endif