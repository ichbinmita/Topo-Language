/**
 * Parser for Topo Programming Language
 * Version 1.3.0
 * Author: Dmitry (Republic of Sakha, Yakutia)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include "lexer.c"  // Include our lexer (без main!)
#include "ast.h"    // Include AST definitions
#include "parser.h" // Include parser prototypes

// ================ PARSER STRUCTURE ================
typedef struct {
    Lexer* lexer;
    Token current;
    bool has_error;
    char error_msg[256];
    int error_line;
    int error_column;
} Parser;

// ================ FORWARD DECLARATIONS ================
static ASTNode* parse_expression(Parser* parser);
static ASTNode* parse_block(Parser* parser);
static ASTNode* parse_statement(Parser* parser);

// ================ PARSER FUNCTIONS ================

// Create parser
Parser* parser_create(Lexer* lexer) {
    Parser* parser = (Parser*)calloc(1, sizeof(Parser));
    if (!parser) return NULL;
    
    parser->lexer = lexer;
    parser->has_error = false;
    parser->current = lexer_current(lexer);
    
    return parser;
}

// Destroy parser
void parser_destroy(Parser* parser) {
    if (!parser) return;
    free(parser);
}

// Error handling
static void parser_error(Parser* parser, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(parser->error_msg, sizeof(parser->error_msg), format, args);
    va_end(args);
    
    parser->has_error = true;
    parser->error_line = parser->current.line;
    parser->error_column = parser->current.column;
    
    fprintf(stderr, "Parse error [%d:%d]: %s\n",
            parser->error_line, parser->error_column, parser->error_msg);
}

// Advance to next token
static void parser_advance(Parser* parser) {
    lexer_skip(parser->lexer);
    parser->current = lexer_current(parser->lexer);
}

// Check current token type
static bool parser_check(Parser* parser, TokenType type) {
    return parser->current.type == type;
}

// Check current token type and value
static bool parser_check_value(Parser* parser, TokenType type, const char* value) {
    if (parser->current.type != type) return false;
    if (!value || !parser->current.value) return !value && !parser->current.value;
    return strcmp(parser->current.value, value) == 0;
}

// Expect specific token (with error message)
static bool parser_expect(Parser* parser, TokenType type, const char* value, const char* error_msg) {
    if (!parser_check_value(parser, type, value)) {
        parser_error(parser, "%s", error_msg);
        return false;
    }
    return true;
}

// Skip a token if it matches
static bool parser_match(Parser* parser, TokenType type, const char* value) {
    if (parser_check_value(parser, type, value)) {
        parser_advance(parser);
        return true;
    }
    return false;
}

// ================ PARSING FUNCTIONS ================

// Parse a literal value
static ASTNode* parse_literal(Parser* parser) {
    Token token = parser->current;
    
    switch (token.type) {
        case TOKEN_NUMBER_INT:
            parser_advance(parser);
            return create_literal_node_int(token.int_val, token.line, token.column);
            
        case TOKEN_NUMBER_FLOAT:
            parser_advance(parser);
            return create_literal_node_float(token.float_val, token.line, token.column);
            
        case TOKEN_STRING:
            parser_advance(parser);
            return create_literal_node_string(token.value, token.line, token.column);
            
        case TOKEN_TRUE:
            parser_advance(parser);
            return create_literal_node_bool(true, token.line, token.column);
            
        case TOKEN_FALSE:
            parser_advance(parser);
            return create_literal_node_bool(false, token.line, token.column);
            
        case TOKEN_NULL:
            parser_advance(parser);
            return create_literal_node_null(token.line, token.column);
            
        default:
            return NULL;
    }
}

// Parse an identifier
static ASTNode* parse_identifier(Parser* parser) {
    if (!parser_check(parser, TOKEN_IDENTIFIER)) {
        return NULL;
    }
    
    Token token = parser->current;
    parser_advance(parser);
    return create_identifier_node(token.value, token.line, token.column);
}

// Parse a primary expression (highest precedence)
static ASTNode* parse_primary(Parser* parser) {
    ASTNode* node = NULL;
    
    // Check for literals first
    node = parse_literal(parser);
    if (node) return node;
    
    // Check for identifier
    node = parse_identifier(parser);
    if (node) {
        // Check for member access (obj.property)
        if (parser_match(parser, TOKEN_PUNCTUATION, ".")) {
            if (!parser_check(parser, TOKEN_IDENTIFIER)) {
                parser_error(parser, "Expected member name after '.'");
                free_ast_node(node);
                return NULL;
            }
            
            Token member_token = parser->current;
            parser_advance(parser);
            
            ASTNode* member_node = create_member_access_node(node, member_token.value, 
                                                            member_token.line, member_token.column);
            return member_node;
        }
        
        // Check for function call
        if (parser_match(parser, TOKEN_PUNCTUATION, "(")) {
            ASTNode* arguments = NULL;
            int arg_count = 0;
            
            // Parse arguments if any
            if (!parser_match(parser, TOKEN_PUNCTUATION, ")")) {
                while (1) {
                    ASTNode* arg = parse_expression(parser);
                    if (!arg) {
                        parser_error(parser, "Expected expression in function call");
                        break;
                    }
                    
                    // Add argument to list
                    if (!arguments) {
                        arguments = arg;
                    } else {
                        add_next_statement(arguments, arg);
                    }
                    arg_count++;
                    
                    if (parser_match(parser, TOKEN_PUNCTUATION, ",")) {
                        continue;
                    }
                    
                    if (parser_match(parser, TOKEN_PUNCTUATION, ")")) {
                        break;
                    }
                    
                    parser_error(parser, "Expected ',' or ')' in function call");
                    break;
                }
            }
            
            node = create_call_expr_node(node, arguments, arg_count, 
                                        parser->current.line, parser->current.column);
        }
        
        return node;
    }
    
    // Check for array literal
    if (parser_match(parser, TOKEN_PUNCTUATION, "[")) {
        ASTNode* elements = NULL;
        int element_count = 0;
        
        // Parse array elements if any
        if (!parser_match(parser, TOKEN_PUNCTUATION, "]")) {
            while (1) {
                ASTNode* element = parse_expression(parser);
                if (!element) {
                    parser_error(parser, "Expected expression in array");
                    break;
                }
                
                // Add element to list
                if (!elements) {
                    elements = element;
                } else {
                    add_next_statement(elements, element);
                }
                element_count++;
                
                if (parser_match(parser, TOKEN_PUNCTUATION, ",")) {
                    continue;
                }
                
                if (parser_match(parser, TOKEN_PUNCTUATION, "]")) {
                    break;
                }
                
                parser_error(parser, "Expected ',' or ']' in array");
                    break;
            }
        }
        
        return create_array_literal_node(elements, element_count, 
                                        parser->current.line, parser->current.column);
    }
    
    // Check for dictionary literal
    if (parser_match(parser, TOKEN_PUNCTUATION, "{")) {
        char** keys = NULL;
        ASTNode* values = NULL;
        int pair_count = 0;
        
        // Parse dictionary pairs if any
        if (!parser_match(parser, TOKEN_PUNCTUATION, "}")) {
            while (1) {
                // Parse key (must be string or identifier)
                char* key = NULL;
                if (parser_check(parser, TOKEN_STRING)) {
                    key = strdup(parser->current.value);
                    parser_advance(parser);
                } else if (parser_check(parser, TOKEN_IDENTIFIER)) {
                    key = strdup(parser->current.value);
                    parser_advance(parser);
                } else {
                    parser_error(parser, "Expected string or identifier as dictionary key");
                    break;
                }
                
                // Expect colon
                if (!parser_expect(parser, TOKEN_PUNCTUATION, ":", "Expected ':' after dictionary key")) {
                    free(key);
                    break;
                }
                
                // Parse value
                ASTNode* value = parse_expression(parser);
                if (!value) {
                    parser_error(parser, "Expected expression as dictionary value");
                    free(key);
                    break;
                }
                
                // Store key and value
                if (!keys) {
                    keys = (char**)malloc(sizeof(char*));
                } else {
                    keys = (char**)realloc(keys, (pair_count + 1) * sizeof(char*));
                }
                
                if (!values) {
                    values = value;
                } else {
                    add_next_statement(values, value);
                }
                
                keys[pair_count] = key;
                pair_count++;
                
                if (parser_match(parser, TOKEN_PUNCTUATION, ",")) {
                    continue;
                }
                
                if (parser_match(parser, TOKEN_PUNCTUATION, "}")) {
                    break;
                }
                
                parser_error(parser, "Expected ',' or '}' in dictionary");
                break;
            }
        }
        
        return create_dict_literal_node(keys, values, pair_count, 
                                       parser->current.line, parser->current.column);
    }
    
    // Check for parenthesized expression
    if (parser_match(parser, TOKEN_PUNCTUATION, "(")) {
        ASTNode* expr = parse_expression(parser);
        if (!expr) {
            parser_error(parser, "Expected expression after '('");
            return NULL;
        }
        
        if (!parser_expect(parser, TOKEN_PUNCTUATION, ")", "Expected ')' after expression")) {
            free_ast_node(expr);
            return NULL;
        }
        
        return expr;
    }
    
    parser_error(parser, "Expected expression");
    return NULL;
}

// Parse unary expression
static ASTNode* parse_unary(Parser* parser) {
    // Check for unary operators
    Token op_token = parser->current;
    char* op = NULL;
    
    if (parser_match(parser, TOKEN_OPERATOR, "-")) {
        op = "-";
    } else if (parser_match(parser, TOKEN_OPERATOR, "!")) {
        op = "!";
    } else if (parser_match(parser, TOKEN_KEYWORD, "not")) {
        op = "not";
    }
    
    if (op) {
        ASTNode* operand = parse_unary(parser);
        if (!operand) {
            parser_error(parser, "Expected operand after unary operator");
            return NULL;
        }
        
        return create_unary_expr_node(op, operand, op_token.line, op_token.column);
    }
    
    return parse_primary(parser);
}

// Parse multiplication/division/modulo expressions
static ASTNode* parse_multiplicative(Parser* parser) {
    ASTNode* left = parse_unary(parser);
    if (!left) return NULL;
    
    while (parser_match(parser, TOKEN_OPERATOR, "*") ||
           parser_match(parser, TOKEN_OPERATOR, "/") ||
           parser_match(parser, TOKEN_OPERATOR, "%")) {
        
        char* op = parser->current.value;
        int line = parser->current.line;
        int column = parser->current.column;
        parser_advance(parser);
        
        ASTNode* right = parse_unary(parser);
        if (!right) {
            parser_error(parser, "Expected right operand for binary operator");
            free_ast_node(left);
            return NULL;
        }
        
        left = create_binary_expr_node(op, left, right, line, column);
    }
    
    return left;
}

// Parse addition/subtraction expressions
static ASTNode* parse_additive(Parser* parser) {
    ASTNode* left = parse_multiplicative(parser);
    if (!left) return NULL;
    
    while (parser_match(parser, TOKEN_OPERATOR, "+") ||
           parser_match(parser, TOKEN_OPERATOR, "-")) {
        
        char* op = parser->current.value;
        int line = parser->current.line;
        int column = parser->current.column;
        parser_advance(parser);
        
        ASTNode* right = parse_multiplicative(parser);
        if (!right) {
            parser_error(parser, "Expected right operand for binary operator");
            free_ast_node(left);
            return NULL;
        }
        
        left = create_binary_expr_node(op, left, right, line, column);
    }
    
    return left;
}

// Parse comparison expressions
static ASTNode* parse_comparison(Parser* parser) {
    ASTNode* left = parse_additive(parser);
    if (!left) return NULL;
    
    while (parser_match(parser, TOKEN_OPERATOR, "<") ||
           parser_match(parser, TOKEN_OPERATOR, ">") ||
           parser_match(parser, TOKEN_OPERATOR, "<=") ||
           parser_match(parser, TOKEN_OPERATOR, ">=") ||
           parser_match(parser, TOKEN_OPERATOR, "==") ||
           parser_match(parser, TOKEN_OPERATOR, "!=")) {
        
        char* op = parser->current.value;
        int line = parser->current.line;
        int column = parser->current.column;
        parser_advance(parser);
        
        ASTNode* right = parse_additive(parser);
        if (!right) {
            parser_error(parser, "Expected right operand for comparison operator");
            free_ast_node(left);
            return NULL;
        }
        
        left = create_binary_expr_node(op, left, right, line, column);
    }
    
    return left;
}

// Parse logical AND expressions
static ASTNode* parse_logical_and(Parser* parser) {
    ASTNode* left = parse_comparison(parser);
    if (!left) return NULL;
    
    while (parser_match(parser, TOKEN_OPERATOR, "&&") ||
           parser_match(parser, TOKEN_KEYWORD, "and")) {
        
        char* op = parser->current.value;
        int line = parser->current.line;
        int column = parser->current.column;
        parser_advance(parser);
        
        ASTNode* right = parse_comparison(parser);
        if (!right) {
            parser_error(parser, "Expected right operand for logical AND");
            free_ast_node(left);
            return NULL;
        }
        
        left = create_binary_expr_node(op, left, right, line, column);
    }
    
    return left;
}

// Parse logical OR expressions
static ASTNode* parse_logical_or(Parser* parser) {
    ASTNode* left = parse_logical_and(parser);
    if (!left) return NULL;
    
    while (parser_match(parser, TOKEN_OPERATOR, "||") ||
           parser_match(parser, TOKEN_KEYWORD, "or")) {
        
        char* op = parser->current.value;
        int line = parser->current.line;
        int column = parser->current.column;
        parser_advance(parser);
        
        ASTNode* right = parse_logical_and(parser);
        if (!right) {
            parser_error(parser, "Expected right operand for logical OR");
            free_ast_node(left);
            return NULL;
        }
        
        left = create_binary_expr_node(op, left, right, line, column);
    }
    
    return left;
}

// Parse assignment expression
static ASTNode* parse_assignment(Parser* parser) {
    ASTNode* left = parse_logical_or(parser);
    if (!left) return NULL;
    
    // Check for assignment operator
    if (parser_match(parser, TOKEN_OPERATOR, "=") ||
        parser_match(parser, TOKEN_OPERATOR, "+=") ||
        parser_match(parser, TOKEN_OPERATOR, "-=") ||
        parser_match(parser, TOKEN_OPERATOR, "*=") ||
        parser_match(parser, TOKEN_OPERATOR, "/=") ||
        parser_match(parser, TOKEN_OPERATOR, "%=")) {
        
        char* op = parser->current.value;
        int line = parser->current.line;
        int column = parser->current.column;
        parser_advance(parser);
        
        // For now, just handle simple assignment
        if (strlen(op) == 2 && op[1] == '=') {
            // Compound assignment - for simplicity, treat as regular assignment
            // In a full implementation, would transform x += 5 to x = x + 5
            parser_error(parser, "Compound assignment not fully implemented yet");
            free_ast_node(left);
            return NULL;
        }
        
        // Simple assignment
        ASTNode* right = parse_assignment(parser);
        if (!right) {
            parser_error(parser, "Expected right side of assignment");
            free_ast_node(left);
            return NULL;
        }
        
        return create_assignment_node(left, right, line, column);
    }
    
    return left;
}

// Main expression parser
static ASTNode* parse_expression(Parser* parser) {
    return parse_assignment(parser);
}

// Parse a block of statements
static ASTNode* parse_block(Parser* parser) {
    ASTNode* statements = NULL;
    ASTNode* last_stmt = NULL;
    
    while (!parser_check(parser, TOKEN_PUNCTUATION) || 
           !parser_check_value(parser, TOKEN_PUNCTUATION, "}")) {
        
        // Skip empty lines
        if (parser_check(parser, TOKEN_NEWLINE)) {
            parser_advance(parser);
            continue;
        }
        
        // Check for end of file
        if (parser_check(parser, TOKEN_EOF)) {
            parser_error(parser, "Unexpected end of file in block");
            break;
        }
        
        ASTNode* stmt = parse_statement(parser);
        if (!stmt) {
            // Try to recover by skipping to next line
            while (!parser_check(parser, TOKEN_NEWLINE) && 
                   !parser_check(parser, TOKEN_EOF)) {
                parser_advance(parser);
            }
            continue;
        }
        
        // Add statement to list
        if (!statements) {
            statements = stmt;
            last_stmt = stmt;
        } else {
            add_next_statement(last_stmt, stmt);
            last_stmt = stmt;
        }
        
        // Skip optional semicolon
        parser_match(parser, TOKEN_PUNCTUATION, ";");
        
        // Skip newline
        if (parser_check(parser, TOKEN_NEWLINE)) {
            parser_advance(parser);
        }
    }
    
    return create_block_node(statements, parser->current.line, parser->current.column);
}

// Parse a statement
static ASTNode* parse_statement(Parser* parser) {
    // Check for various statement types
    
    // Variable declaration
    if (parser_match(parser, TOKEN_VAR, NULL)) {
        if (!parser_check(parser, TOKEN_IDENTIFIER)) {
            parser_error(parser, "Expected variable name after 'var'");
            return NULL;
        }
        
        Token name_token = parser->current;
        parser_advance(parser);
        
        ASTNode* value = NULL;
        if (parser_match(parser, TOKEN_OPERATOR, "=")) {
            value = parse_expression(parser);
            if (!value) {
                parser_error(parser, "Expected expression after '='");
                return NULL;
            }
        }
        
        return create_var_decl_node(name_token.value, value, false, 
                                   name_token.line, name_token.column);
    }
    
    // Constant declaration
    if (parser_match(parser, TOKEN_CONST, NULL)) {
        if (!parser_check(parser, TOKEN_IDENTIFIER)) {
            parser_error(parser, "Expected constant name after 'const'");
            return NULL;
        }
        
        Token name_token = parser->current;
        parser_advance(parser);
        
        if (!parser_expect(parser, TOKEN_OPERATOR, "=", "Expected '=' after constant name")) {
            return NULL;
        }
        
        ASTNode* value = parse_expression(parser);
        if (!value) {
            parser_error(parser, "Expected expression after '='");
            return NULL;
        }
        
        return create_var_decl_node(name_token.value, value, true, 
                                   name_token.line, name_token.column);
    }
    
    // Return statement
    if (parser_match(parser, TOKEN_RETURN, NULL)) {
        ASTNode* value = NULL;
        if (!(parser_check(parser, TOKEN_NEWLINE) || 
              parser_check(parser, TOKEN_EOF) ||
              (parser_check(parser, TOKEN_PUNCTUATION) && 
               parser_check_value(parser, TOKEN_PUNCTUATION, "}")))) {
            value = parse_expression(parser);
        }
        
        return create_return_node(value, parser->current.line, parser->current.column);
    }
    
    // Break statement
    if (parser_match(parser, TOKEN_BREAK, NULL)) {
        return create_break_node(parser->current.line, parser->current.column);
    }
    
    // Continue statement
    if (parser_match(parser, TOKEN_CONTINUE, NULL)) {
        return create_continue_node(parser->current.line, parser->current.column);
    }
    
    // If statement
    if (parser_match(parser, TOKEN_IF, NULL)) {
        // Parse condition (optional parentheses)
        bool has_paren = parser_match(parser, TOKEN_PUNCTUATION, "(");
        
        ASTNode* condition = parse_expression(parser);
        if (!condition) {
            parser_error(parser, "Expected condition after 'if'");
            return NULL;
        }
        
        if (has_paren && !parser_expect(parser, TOKEN_PUNCTUATION, ")", "Expected ')' after condition")) {
            free_ast_node(condition);
            return NULL;
        }
        
        // Optional colon
        parser_match(parser, TOKEN_PUNCTUATION, ":");
        
        // Parse then branch
        ASTNode* then_branch = NULL;
        if (parser_match(parser, TOKEN_PUNCTUATION, "{")) {
            then_branch = parse_block(parser);
            if (!parser_expect(parser, TOKEN_PUNCTUATION, "}", "Expected '}' after block")) {
                free_ast_node(condition);
                free_ast_node(then_branch);
                return NULL;
            }
        } else {
            then_branch = parse_statement(parser);
            if (!then_branch) {
                parser_error(parser, "Expected statement after 'if' condition");
                free_ast_node(condition);
                return NULL;
            }
        }
        
        // Parse elif branches
        ASTNode* elif_branches = NULL;
        while (parser_match(parser, TOKEN_ELIF, NULL)) {
            has_paren = parser_match(parser, TOKEN_PUNCTUATION, "(");
            
            ASTNode* elif_condition = parse_expression(parser);
            if (!elif_condition) {
                parser_error(parser, "Expected condition after 'elif'");
                break;
            }
            
            if (has_paren && !parser_expect(parser, TOKEN_PUNCTUATION, ")", "Expected ')' after condition")) {
                free_ast_node(elif_condition);
                break;
            }
            
            parser_match(parser, TOKEN_PUNCTUATION, ":");
            
            ASTNode* elif_then = NULL;
            if (parser_match(parser, TOKEN_PUNCTUATION, "{")) {
                elif_then = parse_block(parser);
                if (!parser_expect(parser, TOKEN_PUNCTUATION, "}", "Expected '}' after block")) {
                    free_ast_node(elif_condition);
                    free_ast_node(elif_then);
                    break;
                }
            } else {
                elif_then = parse_statement(parser);
                if (!elif_then) {
                    parser_error(parser, "Expected statement after 'elif' condition");
                    free_ast_node(elif_condition);
                    break;
                }
            }
            
            ASTNode* elif_node = create_elif_node(elif_condition, elif_then, 
                                                 parser->current.line, parser->current.column);
            
            if (!elif_branches) {
                elif_branches = elif_node;
            } else {
                add_next_statement(elif_branches, elif_node);
            }
        }
        
        // Parse else branch
        ASTNode* else_branch = NULL;
        if (parser_match(parser, TOKEN_ELSE, NULL)) {
            parser_match(parser, TOKEN_PUNCTUATION, ":");
            
            if (parser_match(parser, TOKEN_PUNCTUATION, "{")) {
                else_branch = parse_block(parser);
                if (!parser_expect(parser, TOKEN_PUNCTUATION, "}", "Expected '}' after block")) {
                    free_ast_node(condition);
                    free_ast_node(then_branch);
                    free_ast_node(elif_branches);
                    return NULL;
                }
            } else {
                else_branch = parse_statement(parser);
                if (!else_branch) {
                    parser_error(parser, "Expected statement after 'else'");
                    free_ast_node(condition);
                    free_ast_node(then_branch);
                    free_ast_node(elif_branches);
                    return NULL;
                }
            }
        }
        
        ASTNode* if_node = create_if_node(condition, then_branch, else_branch,
                                         parser->current.line, parser->current.column);
        
        // Attach elif branches
        if (elif_branches) {
            ASTNode* current = elif_branches;
            while (current) {
                add_next_statement(if_node->flow.elif_branches, current);
                current = current->next;
            }
        }
        
        return if_node;
    }
    
    // While statement
    if (parser_match(parser, TOKEN_WHILE, NULL)) {
        bool has_paren = parser_match(parser, TOKEN_PUNCTUATION, "(");
        
        ASTNode* condition = parse_expression(parser);
        if (!condition) {
            parser_error(parser, "Expected condition after 'while'");
            return NULL;
        }
        
        if (has_paren && !parser_expect(parser, TOKEN_PUNCTUATION, ")", "Expected ')' after condition")) {
            free_ast_node(condition);
            return NULL;
        }
        
        parser_match(parser, TOKEN_PUNCTUATION, ":");
        
        ASTNode* body = NULL;
        if (parser_match(parser, TOKEN_PUNCTUATION, "{")) {
            body = parse_block(parser);
            if (!parser_expect(parser, TOKEN_PUNCTUATION, "}", "Expected '}' after block")) {
                free_ast_node(condition);
                return NULL;
            }
        } else {
            body = parse_statement(parser);
            if (!body) {
                parser_error(parser, "Expected statement after 'while' condition");
                free_ast_node(condition);
                return NULL;
            }
        }
        
        return create_while_node(condition, body, parser->current.line, parser->current.column);
    }
    
    // For statement
    if (parser_match(parser, TOKEN_FOR, NULL)) {
        if (!parser_check(parser, TOKEN_IDENTIFIER)) {
            parser_error(parser, "Expected iterator variable after 'for'");
            return NULL;
        }
        
        Token iterator_token = parser->current;
        parser_advance(parser);
        
        if (!parser_expect(parser, TOKEN_IN, NULL, "Expected 'in' after iterator variable")) {
            return NULL;
        }
        
        ASTNode* iterable = parse_expression(parser);
        if (!iterable) {
            parser_error(parser, "Expected iterable expression after 'in'");
            return NULL;
        }
        
        parser_match(parser, TOKEN_PUNCTUATION, ":");
        
        ASTNode* body = NULL;
        if (parser_match(parser, TOKEN_PUNCTUATION, "{")) {
            body = parse_block(parser);
            if (!parser_expect(parser, TOKEN_PUNCTUATION, "}", "Expected '}' after block")) {
                free_ast_node(iterable);
                return NULL;
            }
        } else {
            body = parse_statement(parser);
            if (!body) {
                parser_error(parser, "Expected statement after 'for' header");
                free_ast_node(iterable);
                return NULL;
            }
        }
        
        return create_for_node(iterator_token.value, iterable, body,
                              parser->current.line, parser->current.column);
    }
    
    // From import statement
    if (parser_match(parser, TOKEN_FROM, NULL)) {
        if (!parser_check(parser, TOKEN_IDENTIFIER)) {
            parser_error(parser, "Expected module name after 'from'");
            return NULL;
        }
        
        Token module_token = parser->current;
        parser_advance(parser);
        
        if (!parser_expect(parser, TOKEN_USING, NULL, "Expected 'using' after module name")) {
            return NULL;
        }
        
        char** imports = NULL;
        int import_count = 0;
        bool import_all = false;
        
        // Check for wildcard import
        if (parser_match(parser, TOKEN_OPERATOR, "*")) {
            import_all = true;
        } else {
            // Parse import list
            while (1) {
                if (!parser_check(parser, TOKEN_IDENTIFIER)) {
                    parser_error(parser, "Expected identifier in import list");
                    break;
                }
                
                char* import_name = strdup(parser->current.value);
                parser_advance(parser);
                
                if (!imports) {
                    imports = (char**)malloc(sizeof(char*));
                } else {
                    imports = (char**)realloc(imports, (import_count + 1) * sizeof(char*));
                }
                
                imports[import_count] = import_name;
                import_count++;
                
                if (parser_match(parser, TOKEN_PUNCTUATION, ",")) {
                    continue;
                }
                
                break;
            }
        }
        
        return create_from_import_node(module_token.value, imports, import_count, 
                                      import_all, parser->current.line, parser->current.column);
    }
    
    // Expression statement (including assignment)
    ASTNode* expr = parse_expression(parser);
    if (expr) {
        return create_expr_stmt_node(expr, parser->current.line, parser->current.column);
    }
    
    parser_error(parser, "Expected statement");
    return NULL;
}

// Parse a complete program
ASTNode* parse_program(Parser* parser) {
    ASTNode* statements = NULL;
    ASTNode* last_stmt = NULL;
    
    while (!parser_check(parser, TOKEN_EOF)) {
        // Skip empty lines
        if (parser_check(parser, TOKEN_NEWLINE)) {
            parser_advance(parser);
            continue;
        }
        
        ASTNode* stmt = parse_statement(parser);
        if (!stmt) {
            // Try to recover by skipping to next line
            while (!parser_check(parser, TOKEN_NEWLINE) && 
                   !parser_check(parser, TOKEN_EOF)) {
                parser_advance(parser);
            }
            continue;
        }
        
        // Add statement to list
        if (!statements) {
            statements = stmt;
            last_stmt = stmt;
        } else {
            add_next_statement(last_stmt, stmt);
            last_stmt = stmt;
        }
        
        // Skip optional semicolon
        parser_match(parser, TOKEN_PUNCTUATION, ";");
        
        // Skip newline
        if (parser_check(parser, TOKEN_NEWLINE)) {
            parser_advance(parser);
        }
    }
    
    return create_program_node(statements);
}

// ================ PUBLIC API ================

// Parse source code into AST
ASTNode* parse_source(const char* source, const char* filename) {
    // Create lexer
    Lexer* lexer = lexer_create(source, filename);
    if (!lexer) {
        fprintf(stderr, "Failed to create lexer\n");
        return NULL;
    }
    
    // Create parser
    Parser* parser = parser_create(lexer);
    if (!parser) {
        lexer_destroy(lexer);
        fprintf(stderr, "Failed to create parser\n");
        return NULL;
    }
    
    // Parse program
    ASTNode* ast = parse_program(parser);
    
    // Check for errors
    if (parser->has_error) {
        fprintf(stderr, "Parsing failed with errors\n");
        free_ast_node(ast);
        ast = NULL;
    }
    
    // Cleanup
    parser_destroy(parser);
    lexer_destroy(lexer);
    
    return ast;
}