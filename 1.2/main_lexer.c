#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include "lexer.c" // Включаем лексер

int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "en_US.UTF-8");
    
    if (argc < 2) {
        printf("Topo Language Lexer 1.3.0 (Test)\n");
        printf("Usage:\n");
        printf("  %s test          # run tests\n", argv[0]);
        printf("  %s file.topo     # analyze file\n", argv[0]);
        printf("  %s -e \"code\"     # analyze code from command line\n\n", argv[0]);
        
        test_lexer();
        return 0;
    }
    
    if (strcmp(argv[1], "test") == 0) {
        test_lexer();
        return 0;
    }
    
    if (strcmp(argv[1], "-e") == 0 && argc >= 3) {
        printf("=== Analyzing code from command line ===\n\n");
        
        Lexer* lexer = lexer_create(argv[2], "<command-line>");
        
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
        
        lexer_destroy(lexer);
        return 0;
    }
    
    // Read from file
    FILE* file = fopen(argv[1], "rb");
    if (!file) {
        fprintf(stderr, "Error: cannot open file '%s'\n", argv[1]);
        return 1;
    }
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* source = (char*)malloc(file_size + 1);
    if (!source) {
        fclose(file);
        fprintf(stderr, "Error: cannot allocate memory\n");
        return 1;
    }
    
    fread(source, 1, file_size, file);
    source[file_size] = '\0';
    fclose(file);
    
    printf("=== Analyzing file: %s ===\n\n", argv[1]);
    
    Lexer* lexer = lexer_create(source, argv[1]);
    
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
    
    if (lexer->has_error) {
        printf("Errors found!\n");
    }
    
    free(source);
    lexer_destroy(lexer);
    
    return 0;
}