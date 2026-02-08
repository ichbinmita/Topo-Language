/**
 * Lexer for Topo Programming Language
 * Version 1.3.0 with full UTF-8 support
 * Author: Dmitry (Republic of Sakha, Yakutia)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <locale.h>
#include <stdarg.h>  // Added for va_start, va_end
#include <wchar.h>
#include <wctype.h>

// ================ CONSTANTS ================
#define MAX_TOKEN_LENGTH 256
#define MAX_STRING_LENGTH 4096
#define MAX_IDENTIFIER_LENGTH 128
#define KEYWORD_COUNT 28
#define OPERATOR_COUNT 20
#define MAX_LOOKAHEAD 2

// ================ DATA TYPES ================
typedef enum {
    // Keywords
    TOKEN_VAR, TOKEN_CONST, TOKEN_FUNC, TOKEN_IF, TOKEN_ELSE, TOKEN_ELIF,
    TOKEN_WHILE, TOKEN_FOR, TOKEN_IN, TOKEN_RETURN, TOKEN_TRUE, TOKEN_FALSE,
    TOKEN_NULL, TOKEN_AND, TOKEN_OR, TOKEN_NOT, TOKEN_BREAK, TOKEN_CONTINUE,
    
    // Built-in functions
    TOKEN_CONSOLE, TOKEN_INPUT, TOKEN_LEN, TOKEN_APPEND, TOKEN_POP,
    TOKEN_KEYS, TOKEN_VALUES, TOKEN_TYPE, TOKEN_INT, TOKEN_FLOAT_FUNC, // Changed from TOKEN_FLOAT
    TOKEN_STR, TOKEN_BOOL, TOKEN_ARRAY, TOKEN_DICT, TOKEN_RANGE,
    TOKEN_FROM, TOKEN_USING,
    
    // Token types
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER_INT,
    TOKEN_NUMBER_FLOAT,
    TOKEN_STRING,
    TOKEN_OPERATOR,
    TOKEN_PUNCTUATION,
    TOKEN_NEWLINE,
    TOKEN_EOF,
    TOKEN_ERROR
} TokenType;

// Operator table
typedef struct {
    const char* str;
    int length;
    TokenType type;
} Operator;

// Token structure
typedef struct {
    TokenType type;
    char* value;           // For strings, numbers, identifiers
    long int_val;          // For integers
    double float_val;      // For floating point numbers
    int line;
    int column;
    int length;
} Token;

// Lexer structure
typedef struct {
    const char* source;    // Source code
    const char* filename;  // File name (for debugging)
    int position;          // Current position
    int line;              // Current line (1-based)
    int column;            // Current column (1-based)
    int start_position;    // Start of current token
    int start_line;        // Line start of token
    int start_column;      // Column start of token
    Token current;         // Current token
    Token lookahead[MAX_LOOKAHEAD]; // Lookahead buffer
    int lookahead_pos;
    bool has_error;
    char error_msg[256];
    
    // Buffers
    char string_buffer[MAX_STRING_LENGTH];
    int string_buffer_pos;
} Lexer;

// ================ GLOBAL TABLES ================

// Keywords
static const struct {
    const char* keyword;
    TokenType type;
} keyword_table[] = {
    // Declarations
    {"var", TOKEN_VAR},
    {"const", TOKEN_CONST},
    {"func", TOKEN_FUNC},
    
    // Flow control
    {"if", TOKEN_IF},
    {"else", TOKEN_ELSE},
    {"elif", TOKEN_ELIF},
    {"while", TOKEN_WHILE},
    {"for", TOKEN_FOR},
    {"in", TOKEN_IN},
    {"return", TOKEN_RETURN},
    {"break", TOKEN_BREAK},
    {"continue", TOKEN_CONTINUE},
    
    // Literals
    {"true", TOKEN_TRUE},
    {"false", TOKEN_FALSE},
    {"null", TOKEN_NULL},
    
    // Logical operators
    {"and", TOKEN_AND},
    {"or", TOKEN_OR},
    {"not", TOKEN_NOT},
    
    // Built-in functions
    {"console", TOKEN_CONSOLE},
    {"input", TOKEN_INPUT},
    {"len", TOKEN_LEN},
    {"append", TOKEN_APPEND},
    {"pop", TOKEN_POP},
    {"keys", TOKEN_KEYS},
    {"values", TOKEN_VALUES},
    {"type", TOKEN_TYPE},
    {"int", TOKEN_INT},
    {"float", TOKEN_FLOAT_FUNC}, // Changed from TOKEN_FLOAT
    {"str", TOKEN_STR},
    {"bool", TOKEN_BOOL},
    {"array", TOKEN_ARRAY},
    {"dict", TOKEN_DICT},
    {"range", TOKEN_RANGE},
    {"from", TOKEN_FROM},
    {"using", TOKEN_USING},
    
    {NULL, TOKEN_IDENTIFIER}
};

// Operators (sorted by length for correct recognition)
static const Operator operator_table[] = {
    // Two-character
    {"==", 2, TOKEN_OPERATOR},
    {"!=", 2, TOKEN_OPERATOR},
    {"<=", 2, TOKEN_OPERATOR},
    {">=", 2, TOKEN_OPERATOR},
    {"&&", 2, TOKEN_OPERATOR},
    {"||", 2, TOKEN_OPERATOR},
    {"+=", 2, TOKEN_OPERATOR},
    {"-=", 2, TOKEN_OPERATOR},
    {"*=", 2, TOKEN_OPERATOR},
    {"/=", 2, TOKEN_OPERATOR},
    {"%=", 2, TOKEN_OPERATOR},
    
    // One-character
    {"+", 1, TOKEN_OPERATOR},
    {"-", 1, TOKEN_OPERATOR},
    {"*", 1, TOKEN_OPERATOR},
    {"/", 1, TOKEN_OPERATOR},
    {"%", 1, TOKEN_OPERATOR},
    {"=", 1, TOKEN_OPERATOR},
    {"<", 1, TOKEN_OPERATOR},
    {">", 1, TOKEN_OPERATOR},
    {"!", 1, TOKEN_OPERATOR},
    {"&", 1, TOKEN_OPERATOR},
    {"|", 1, TOKEN_OPERATOR},
    {"^", 1, TOKEN_OPERATOR},
    {"~", 1, TOKEN_OPERATOR},
    
    {NULL, 0, TOKEN_OPERATOR}
};

// Punctuation
static const char* punctuation_chars = "(){}[].,;:";

// ================ UTF-8 UTILITIES ================

// Get UTF-8 character length in bytes
static int utf8_char_length(unsigned char c) {
    if ((c & 0x80) == 0x00) return 1;          // 0xxxxxxx
    if ((c & 0xE0) == 0xC0) return 2;          // 110xxxxx
    if ((c & 0xF0) == 0xE0) return 3;          // 1110xxxx
    if ((c & 0xF8) == 0xF0) return 4;          // 11110xxx
    return 1; // Invalid UTF-8, treat as is
}

// Check if character is valid for identifier start (with UTF-8 support)
static bool is_identifier_start(char c) {
    unsigned char uc = (unsigned char)c;
    
    // ASCII letters and underscore
    if (isalpha(uc) || uc == '_') return true;
    
    // UTF-8 start (non-ASCII characters, including Cyrillic, Yakut letters)
    if (uc >= 0xC0) {  // UTF-8 character start
        // Check if this is a valid UTF-8 start
        int len = utf8_char_length(uc);
        return len >= 2;  // Accept all multi-byte UTF-8 characters
    }
    
    return false;
}

static bool is_identifier_char(char c) {
    unsigned char uc = (unsigned char)c;
    
    // ASCII letters, digits, underscore
    if (isalnum(uc) || uc == '_') return true;
    
    // For UTF-8: check if continuation
    if (uc >= 0x80) {
        return true;  // Accept all UTF-8 characters in identifiers
    }
    
    return false;
}

// ================ LEXER FUNCTIONS ================

// Create lexer
Lexer* lexer_create(const char* source, const char* filename) {
    Lexer* lexer = (Lexer*)calloc(1, sizeof(Lexer));
    if (!lexer) return NULL;
    
    lexer->source = source;
    lexer->filename = filename ? strdup(filename) : NULL;
    lexer->position = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->start_position = 0;
    lexer->start_line = 1;
    lexer->start_column = 1;
    lexer->has_error = false;
    lexer->lookahead_pos = 0;
    lexer->string_buffer_pos = 0;
    
    // Initialize current token
    lexer->current.type = TOKEN_EOF;
    lexer->current.value = NULL;
    lexer->current.line = 1;
    lexer->current.column = 1;
    lexer->current.length = 0;
    
    return lexer;
}

// Destroy lexer
void lexer_destroy(Lexer* lexer) {
    if (!lexer) return;
    
    // Free token memory
    if (lexer->current.value) {
        free(lexer->current.value);
    }
    
    for (int i = 0; i < MAX_LOOKAHEAD; i++) {
        if (lexer->lookahead[i].value) {
            free(lexer->lookahead[i].value);
        }
    }
    
    if (lexer->filename) {
        free((void*)lexer->filename);
    }
    
    free(lexer);
}

// Set error
static void lexer_error(Lexer* lexer, const char* format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(lexer->error_msg, sizeof(lexer->error_msg), format, args);
    va_end(args);
    
    lexer->has_error = true;
    fprintf(stderr, "Lexical error [%s:%d:%d]: %s\n",
            lexer->filename ? lexer->filename : "<source>",
            lexer->line, lexer->column, lexer->error_msg);
}

// Get current character
static char lexer_peek(Lexer* lexer) {
    if (!lexer || !lexer->source) return '\0';
    return lexer->source[lexer->position];
}

// Get character at offset
static char lexer_peek_at(Lexer* lexer, int offset) {
    if (!lexer || !lexer->source) return '\0';
    if (lexer->position + offset < 0) return '\0';
    return lexer->source[lexer->position + offset];
}

// Advance to next character
static char lexer_advance(Lexer* lexer) {
    if (!lexer || !lexer->source) return '\0';
    
    char c = lexer->source[lexer->position];
    if (c == '\0') return '\0';
    
    lexer->position++;
    
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    
    return c;
}

// Skip whitespace
static void lexer_skip_whitespace(Lexer* lexer) {
    while (lexer->source[lexer->position] != '\0') {
        char c = lexer->source[lexer->position];
        
        if (c == ' ' || c == '\t' || c == '\r') {
            lexer_advance(lexer);
        } else {
            break;
        }
    }
}

// Start new token
static void lexer_start_token(Lexer* lexer) {
    lexer->start_position = lexer->position;
    lexer->start_line = lexer->line;
    lexer->start_column = lexer->column;
}

// Create token
static Token lexer_make_token(Lexer* lexer, TokenType type, const char* value) {
    Token token;
    token.type = type;
    token.value = value ? strdup(value) : NULL;
    token.line = lexer->start_line;
    token.column = lexer->start_column;
    token.length = lexer->position - lexer->start_position;
    token.int_val = 0;
    token.float_val = 0.0;
    
    return token;
}

// Create number token with correct parsing
static Token lexer_make_number_token(Lexer* lexer, const char* text, bool is_float, bool is_hex, bool is_binary) {
    Token token = lexer_make_token(lexer, 
        is_float ? TOKEN_NUMBER_FLOAT : TOKEN_NUMBER_INT, text);
    
    if (is_float) {
        token.float_val = atof(text);
    } else {
        if (is_hex) {
            token.int_val = strtol(text, NULL, 16);
        } else if (is_binary) {
            token.int_val = strtol(text + 2, NULL, 2); // Skip "0b" prefix
        } else {
            token.int_val = atol(text);
        }
    }
    
    return token;
}

// Process single-line comments
static bool lexer_skip_line_comment(Lexer* lexer) {
    if (lexer_peek_at(lexer, 0) == '/' && lexer_peek_at(lexer, 1) == '/') {
        lexer_advance(lexer); // /
        lexer_advance(lexer); // /
        
        while (lexer_peek(lexer) != '\n' && lexer_peek(lexer) != '\0') {
            lexer_advance(lexer);
        }
        return true;
    }
    return false;
}

// Process multi-line comments
static bool lexer_skip_block_comment(Lexer* lexer) {
    if (lexer_peek_at(lexer, 0) == '/' && lexer_peek_at(lexer, 1) == '*') {
        lexer_advance(lexer); // /
        lexer_advance(lexer); // *
        
        int depth = 1;
        while (depth > 0 && lexer_peek(lexer) != '\0') {
            if (lexer_peek_at(lexer, 0) == '/' && lexer_peek_at(lexer, 1) == '*') {
                depth++;
                lexer_advance(lexer); // /
                lexer_advance(lexer); // *
            } else if (lexer_peek_at(lexer, 0) == '*' && lexer_peek_at(lexer, 1) == '/') {
                depth--;
                lexer_advance(lexer); // *
                lexer_advance(lexer); // /
            } else {
                if (lexer_peek(lexer) == '\n') {
                    lexer->line++;
                    lexer->column = 0;
                }
                lexer_advance(lexer);
            }
        }
        
        if (depth > 0) {
            lexer_error(lexer, "Unclosed multi-line comment");
        }
        return true;
    }
    return false;
}

// Process numbers
static Token lexer_read_number(Lexer* lexer) {
    lexer_start_token(lexer);
    
    bool is_float = false;
    bool has_exponent = false;
    bool is_hex = false;
    bool is_binary = false;
    
    // Check for hexadecimal and binary numbers
    if (lexer_peek(lexer) == '0') {
        char next = lexer_peek_at(lexer, 1);
        if (next == 'x' || next == 'X') {
            is_hex = true;
            lexer_advance(lexer); // 0
            lexer_advance(lexer); // x/X
        } else if (next == 'b' || next == 'B') {
            is_binary = true;
            lexer_advance(lexer); // 0
            lexer_advance(lexer); // b/B
        }
    }
    
    while (1) {
        char c = lexer_peek(lexer);
        
        if (is_hex) {
            if (!isxdigit((unsigned char)c)) break;
        } else if (is_binary) {
            if (c != '0' && c != '1') break;
        } else {
            if (c == '.') {
                if (is_float || has_exponent) {
                    lexer_error(lexer, "Invalid number format");
                    break;
                }
                is_float = true;
            } else if (c == 'e' || c == 'E') {
                if (has_exponent) {
                    lexer_error(lexer, "Invalid number format");
                    break;
                }
                has_exponent = true;
                is_float = true; // Numbers with exponent are always floating point
                
                // Check next character for +/-
                char next = lexer_peek_at(lexer, 1);
                if (next == '+' || next == '-') {
                    lexer_advance(lexer); // e/E
                    lexer_advance(lexer); // +/-
                    continue;
                }
            } else if (!isdigit((unsigned char)c)) {
                break;
            }
        }
        
        lexer_advance(lexer);
    }
    
    // Extract number text
    int length = lexer->position - lexer->start_position;
    char* number_text = (char*)malloc(length + 1);
    if (!number_text) {
        Token error_token = lexer_make_token(lexer, TOKEN_ERROR, "Memory error");
        error_token.length = 0;
        return error_token;
    }
    
    strncpy(number_text, lexer->source + lexer->start_position, length);
    number_text[length] = '\0';
    
    Token token = lexer_make_number_token(lexer, number_text, is_float, is_hex, is_binary);
    free(number_text);
    
    return token;
}

// Process string literals
static Token lexer_read_string(Lexer* lexer) {
    lexer_start_token(lexer);
    
    char quote = lexer_advance(lexer); // Skip opening quote
    lexer->string_buffer_pos = 0;
    
    while (lexer_peek(lexer) != quote && lexer_peek(lexer) != '\0') {
        char c = lexer_advance(lexer);
        
        if (c == '\\') { // Escape sequence
            char next = lexer_advance(lexer);
            switch (next) {
                case 'n': c = '\n'; break;
                case 't': c = '\t'; break;
                case 'r': c = '\r'; break;
                case '"': c = '"'; break;
                case '\'': c = '\''; break;
                case '\\': c = '\\'; break;
                case '0': c = '\0'; break;
                case 'x': { // \xHH
                    char hex[3] = {0};
                    hex[0] = lexer_advance(lexer);
                    hex[1] = lexer_advance(lexer);
                    c = (char)strtol(hex, NULL, 16);
                    break;
                }
                case 'u': { // \uXXXX (Unicode)
                    char hex[5] = {0};
                    for (int i = 0; i < 4; i++) {
                        hex[i] = lexer_advance(lexer);
                        if (!isxdigit((unsigned char)hex[i])) {
                            lexer_error(lexer, "Invalid Unicode escape");
                            c = '?';
                            break;
                        }
                    }
                    // In this version just skip Unicode
                    c = '?'; // Placeholder
                    break;
                }
                default:
                    lexer_error(lexer, "Unknown escape sequence: \\%c", next);
                    break;
            }
        }
        
        if (lexer->string_buffer_pos >= MAX_STRING_LENGTH - 1) {
            lexer_error(lexer, "String too long");
            break;
        }
        
        lexer->string_buffer[lexer->string_buffer_pos++] = c;
    }
    
    if (lexer_peek(lexer) != quote) {
        lexer_error(lexer, "Unclosed string");
        Token error_token = lexer_make_token(lexer, TOKEN_ERROR, "Unclosed string");
        error_token.length = 0;
        return error_token;
    }
    
    lexer_advance(lexer); // Skip closing quote
    
    lexer->string_buffer[lexer->string_buffer_pos] = '\0';
    return lexer_make_token(lexer, TOKEN_STRING, lexer->string_buffer);
}

// Process identifiers and keywords
static Token lexer_read_identifier(Lexer* lexer) {
    lexer_start_token(lexer);
    
    // Read first character (already checked for is_identifier_start)
    lexer_advance(lexer);
    
    // Read remaining characters
    while (is_identifier_char(lexer_peek(lexer))) {
        // Check identifier length
        if (lexer->position - lexer->start_position >= MAX_IDENTIFIER_LENGTH) {
            lexer_error(lexer, "Identifier too long");
            break;
        }
        lexer_advance(lexer);
    }
    
    // Extract identifier text
    int length = lexer->position - lexer->start_position;
    char* text = (char*)malloc(length + 1);
    if (!text) {
        Token error_token = lexer_make_token(lexer, TOKEN_ERROR, "Memory error");
        error_token.length = 0;
        return error_token;
    }
    
    strncpy(text, lexer->source + lexer->start_position, length);
    text[length] = '\0';
    
    // Check if it's a keyword
    for (int i = 0; keyword_table[i].keyword != NULL; i++) {
        if (strcmp(text, keyword_table[i].keyword) == 0) {
            free(text);
            return lexer_make_token(lexer, keyword_table[i].type, NULL);
        }
    }
    
    Token token = lexer_make_token(lexer, TOKEN_IDENTIFIER, text);
    free(text);
    
    return token;
}

// Process operators
static Token lexer_read_operator(Lexer* lexer) {
    lexer_start_token(lexer);
    
    // Try to find the longest matching operator
    const Operator* best_op = NULL;
    
    for (int i = 0; operator_table[i].str != NULL; i++) {
        const Operator* op = &operator_table[i];
        bool match = true;
        
        // Check if operator matches
        for (int j = 0; j < op->length; j++) {
            if (lexer_peek_at(lexer, j) != op->str[j]) {
                match = false;
                break;
            }
        }
        
        if (match) {
            // Select the longest matching operator
            if (!best_op || op->length > best_op->length) {
                best_op = op;
            }
        }
    }
    
    if (best_op) {
        for (int i = 0; i < best_op->length; i++) {
            lexer_advance(lexer);
        }
        return lexer_make_token(lexer, TOKEN_OPERATOR, best_op->str);
    }
    
    // If operator not found, it's an error
    char c = lexer_peek(lexer);
    lexer_error(lexer, "Unknown operator: '%c'", c);
    lexer_advance(lexer);
    Token error_token = lexer_make_token(lexer, TOKEN_ERROR, "Unknown operator");
    error_token.length = 0;
    return error_token;
}

// Main function to read next token
Token lexer_next(Lexer* lexer) {
    // If there are lookahead tokens, return them
    if (lexer->lookahead_pos > 0) {
        Token token = lexer->lookahead[0];
        
        // Shift lookahead buffer
        for (int i = 0; i < MAX_LOOKAHEAD - 1; i++) {
            lexer->lookahead[i] = lexer->lookahead[i + 1];
        }
        lexer->lookahead_pos--;
        
        // Free memory in shifted element
        if (lexer->lookahead[MAX_LOOKAHEAD - 1].value) {
            free(lexer->lookahead[MAX_LOOKAHEAD - 1].value);
            lexer->lookahead[MAX_LOOKAHEAD - 1].value = NULL;
        }
        
        return token;
    }
    
    // Skip whitespace and comments
    while (1) {
        lexer_skip_whitespace(lexer);
        
        // Skip comments
        if (lexer_skip_line_comment(lexer) || lexer_skip_block_comment(lexer)) {
            continue;
        }
        
        // Check for end of file
        if (lexer_peek(lexer) == '\0') {
            return lexer_make_token(lexer, TOKEN_EOF, NULL);
        }
        
        // Check for newline
        if (lexer_peek(lexer) == '\n') {
            lexer_start_token(lexer);
            lexer_advance(lexer);
            return lexer_make_token(lexer, TOKEN_NEWLINE, "\n");
        }
        
        break;
    }
    
    char c = lexer_peek(lexer);
    
    // Numbers
    if (isdigit((unsigned char)c) || 
        (c == '.' && isdigit((unsigned char)lexer_peek_at(lexer, 1)))) {
        return lexer_read_number(lexer);
    }
    
    // Strings
    if (c == '"' || c == '\'') {
        return lexer_read_string(lexer);
    }
    
    // Identifiers and keywords
    if (is_identifier_start(c)) {
        return lexer_read_identifier(lexer);
    }
    
    // Operators
    for (int i = 0; operator_table[i].str != NULL; i++) {
        if (lexer_peek_at(lexer, 0) == operator_table[i].str[0]) {
            return lexer_read_operator(lexer);
        }
    }
    
    // Punctuation
    if (strchr(punctuation_chars, c) != NULL) {
        lexer_start_token(lexer);
        char punct[2] = {lexer_advance(lexer), '\0'};
        return lexer_make_token(lexer, TOKEN_PUNCTUATION, punct);
    }
    
    // Unknown character
    lexer_start_token(lexer);
    lexer_error(lexer, "Unknown character: '%c' (0x%02x)", c, (unsigned char)c);
    lexer_advance(lexer);
    Token error_token = lexer_make_token(lexer, TOKEN_ERROR, "Unknown character");
    error_token.length = 0;
    return error_token;
}

// Token lookahead function
Token lexer_peek_token(Lexer* lexer, int lookahead) {
    if (lookahead < 0 || lookahead >= MAX_LOOKAHEAD) {
        Token error_token;
        error_token.type = TOKEN_ERROR;
        error_token.value = NULL;
        error_token.line = 0;
        error_token.column = 0;
        error_token.length = 0;
        error_token.int_val = 0;
        error_token.float_val = 0.0;
        return error_token;
    }
    
    // Fill lookahead buffer if needed
    while (lexer->lookahead_pos <= lookahead) {
        Token token = lexer_next(lexer);
        
        // Clone token value
        if (token.value) {
            token.value = strdup(token.value);
        }
        
        lexer->lookahead[lexer->lookahead_pos++] = token;
    }
    
    return lexer->lookahead[lookahead];
}

// Skip token
void lexer_skip(Lexer* lexer) {
    if (lexer->lookahead_pos > 0) {
        // Free memory of first token in buffer
        if (lexer->lookahead[0].value) {
            free(lexer->lookahead[0].value);
        }
        
        // Shift buffer
        for (int i = 0; i < MAX_LOOKAHEAD - 1; i++) {
            lexer->lookahead[i] = lexer->lookahead[i + 1];
        }
        lexer->lookahead_pos--;
        
        // Clear last element
        if (lexer->lookahead[MAX_LOOKAHEAD - 1].value) {
            free(lexer->lookahead[MAX_LOOKAHEAD - 1].value);
            lexer->lookahead[MAX_LOOKAHEAD - 1].value = NULL;
        }
    } else {
        // Free memory of current token
        if (lexer->current.value) {
            free(lexer->current.value);
            lexer->current.value = NULL;
        }
        
        // Read next token
        lexer->current = lexer_next(lexer);
    }
}

// Get current token
Token lexer_current(Lexer* lexer) {
    if (lexer->lookahead_pos > 0) {
        return lexer->lookahead[0];
    }
    return lexer->current;
}

// Check token type
bool lexer_check(Lexer* lexer, TokenType type) {
    Token token = lexer_current(lexer);
    return token.type == type;
}

// Check token type and value
bool lexer_check_value(Lexer* lexer, TokenType type, const char* value) {
    Token token = lexer_current(lexer);
    if (token.type != type) return false;
    if (!value || !token.value) return !value && !token.value;
    return strcmp(token.value, value) == 0;
}

// Expect specific token (with skip)
bool lexer_expect(Lexer* lexer, TokenType type, const char* value, const char* error_msg) {
    Token token = lexer_current(lexer);
    
    bool match = true;
    if (type != TOKEN_ERROR && token.type != type) match = false;
    if (value && (!token.value || strcmp(token.value, value) != 0)) match = false;
    
    if (!match) {
        if (error_msg) {
            lexer_error(lexer, "%s (expected %s, got %s)", 
                       error_msg, 
                       value ? value : "token", 
                       token.value ? token.value : "EOF");
        }
        return false;
    }
    
    lexer_skip(lexer);
    return true;
}

// ================ DEBUG UTILITIES ================

// Get token type name
const char* token_type_name(TokenType type) {
    switch (type) {
        case TOKEN_VAR: return "VAR";
        case TOKEN_CONST: return "CONST";
        case TOKEN_FUNC: return "FUNC";
        case TOKEN_IF: return "IF";
        case TOKEN_ELSE: return "ELSE";
        case TOKEN_ELIF: return "ELIF";
        case TOKEN_WHILE: return "WHILE";
        case TOKEN_FOR: return "FOR";
        case TOKEN_IN: return "IN";
        case TOKEN_RETURN: return "RETURN";
        case TOKEN_TRUE: return "TRUE";
        case TOKEN_FALSE: return "FALSE";
        case TOKEN_NULL: return "NULL";
        case TOKEN_AND: return "AND";
        case TOKEN_OR: return "OR";
        case TOKEN_NOT: return "NOT";
        case TOKEN_BREAK: return "BREAK";
        case TOKEN_CONTINUE: return "CONTINUE";
        case TOKEN_CONSOLE: return "CONSOLE";
        case TOKEN_INPUT: return "INPUT";
        case TOKEN_LEN: return "LEN";
        case TOKEN_APPEND: return "APPEND";
        case TOKEN_POP: return "POP";
        case TOKEN_KEYS: return "KEYS";
        case TOKEN_VALUES: return "VALUES";
        case TOKEN_TYPE: return "TYPE";
        case TOKEN_INT: return "INT_FUNC";
        case TOKEN_FLOAT_FUNC: return "FLOAT_FUNC";
        case TOKEN_STR: return "STR_FUNC";
        case TOKEN_BOOL: return "BOOL_FUNC";
        case TOKEN_ARRAY: return "ARRAY_FUNC";
        case TOKEN_DICT: return "DICT_FUNC";
        case TOKEN_RANGE: return "RANGE";
        case TOKEN_FROM: return "FROM";
        case TOKEN_USING: return "USING";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_NUMBER_INT: return "NUMBER_INT";
        case TOKEN_NUMBER_FLOAT: return "NUMBER_FLOAT";
        case TOKEN_STRING: return "STRING";
        case TOKEN_OPERATOR: return "OPERATOR";
        case TOKEN_PUNCTUATION: return "PUNCTUATION";
        case TOKEN_NEWLINE: return "NEWLINE";
        case TOKEN_EOF: return "EOF";
        case TOKEN_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

// Print token
void token_print(const Token* token) {
    printf("[%s", token_type_name(token->type));
    
    if (token->value) {
        printf(" '%s'", token->value);
    }
    
    if (token->type == TOKEN_NUMBER_INT) {
        printf(" (value=%ld)", token->int_val);
    } else if (token->type == TOKEN_NUMBER_FLOAT) {
        printf(" (value=%g)", token->float_val);
    }
    
    printf(" at %d:%d]", token->line, token->column);
}

// ================ TEST FUNCTION ================

void test_lexer() {
    printf("=== Topo Language Lexer Test 1.3.0 ===\n\n");
    
    // Test code with UTF-8 support
    const char* test_code = 
        "// Comment\n"
        "/* Multi-line\n"
        "   comment */\n"
        "\n"
        "// Variable declarations\n"
        "var name = \"mita\"\n"
        "const age = 0\n"
        "var spisok = [1, 2, 3, \"four\"]\n"
        "\n"
        "// Function\n"
        "func hello(name) {\n"
        "    console(\"Hello, \" + name + \"!\")\n"
        "    return true\n"
        "}\n"
        "\n"
        "// Conditions\n"
        "if (age >= 18) {\n"
        "    console(\"Adult\")\n"
        "} elif (age >= 14) {\n"
        "    console(\"Teenager\")\n"
        "} else {\n"
        "    console(\"Child\")\n"
        "}\n"
        "\n"
        "// Loops\n"
        "for i in range(5) {\n"
        "    console(i)\n"
        "}\n"
        "\n"
        "var j = 0\n"
        "while (j < 3) {\n"
        "    console(\"j =\", j)\n"
        "    j = j + 1\n"
        "}\n"
        "\n"
        "// Math\n"
        "var x = 10 + 20 * 3\n"
        "var y = math.sin(math.PI / 2)\n"
        "\n"
        "// Comparison operators\n"
        "var equal = (10 == 10)\n"
        "var notEqual = (10 != 5)\n"
        "var andOp = true and false\n"
        "var orOp = true or false\n"
        "var notOp = not true\n"
        "\n"
        "// Different number formats\n"
        "var hex = 0xFF\n"
        "var binary = 0b1010\n"
        "var floatNum = 3.14e-10\n"
        "\n"
        "// Module imports\n"
        "from math using sin, cos, PI\n"
        "from random using num, choice\n"
        "\n"
        "// Strings with escapes\n"
        "var str = \"String with \\\"quotes\\\" and\\nnewline\"\n"
        "\n"
        "// End of file\n";
    
    Lexer* lexer = lexer_create(test_code, "test.topo");
    
    printf("Source code:\n");
    printf("------------\n%s\n------------\n\n", test_code);
    
    printf("Tokens:\n");
    printf("-------\n");
    
    int token_count = 0;
    Token token;
    
    do {
        token = lexer_next(lexer);
        token_count++;
        
        printf("%3d. ", token_count);
        token_print(&token);
        printf("\n");
        
        if (token.value) {
            free(token.value);
        }
        
    } while (token.type != TOKEN_EOF && token.type != TOKEN_ERROR);
    
    printf("\nTotal tokens: %d\n", token_count);
    printf("Errors: %s\n", lexer->has_error ? "YES" : "no");
    
    if (lexer->has_error) {
        printf("Error message: %s\n", lexer->error_msg);
    }
    
    lexer_destroy(lexer);
}