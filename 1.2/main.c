/**
 * Main test program for Topo Language Parser
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include "ast.c"    // AST implementation
#include "parser.c" // Parser implementation

// Test function
void test_parser() {
    printf("=== Topo Language Parser Test 1.3.0 ===\n\n");
    
    // Test code
    const char* test_code = 
        "var x = 10\n"
        "const y = 20\n"
        "\n"
        "if (x > 5) {\n"
        "    console(\"x is greater than 5\")\n"
        "} else {\n"
        "    console(\"x is 5 or less\")\n"
        "}\n"
        "\n"
        "for i in range(10) {\n"
        "    console(i)\n"
        "}\n"
        "\n"
        "from math using sin, cos\n";
    
    printf("Source code:\n");
    printf("------------\n%s\n------------\n\n", test_code);
    
    // Parse the code
    ASTNode* ast = parse_source(test_code, "test.topo");
    
    if (ast) {
        printf("Parsing successful!\n");
        printf("\nAST Structure:\n");
        printf("--------------\n");
        print_ast(ast, 0);
        
        // Cleanup
        free_ast_node(ast);
    } else {
        printf("Parsing failed!\n");
    }
}

// Main function
int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "en_US.UTF-8");
    
    if (argc < 2) {
        printf("Topo Language Parser 1.3.0\n");
        printf("Author: Dmitry, Republic of Sakha (Yakutia)\n");
        printf("Created for Topo Programming Language project\n\n");
        
        printf("Usage:\n");
        printf("  %s test          # run parser tests\n", argv[0]);
        printf("  %s file.topo     # parse file\n", argv[0]);
        printf("  %s -e \"code\"     # parse code from command line\n\n", argv[0]);
        
        test_parser();
        return 0;
    }
    
    if (strcmp(argv[1], "test") == 0) {
        test_parser();
        return 0;
    }
    
    if (strcmp(argv[1], "-e") == 0 && argc >= 3) {
        printf("=== Parsing code from command line ===\n\n");
        
        ASTNode* ast = parse_source(argv[2], "<command-line>");
        
        if (ast) {
            printf("Parsing successful!\n");
            printf("\nAST Structure:\n");
            printf("--------------\n");
            print_ast(ast, 0);
            
            free_ast_node(ast);
        } else {
            printf("Parsing failed!\n");
        }
        
        return 0;
    }
    
    // Read from file
    FILE* file = fopen(argv[1], "rb");
    if (!file) {
        fprintf(stderr, "Error: cannot open file '%s'\n", argv[1]);
        return 1;
    }
    
    // Determine file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Read file
    char* source = (char*)malloc(file_size + 1);
    if (!source) {
        fclose(file);
        fprintf(stderr, "Error: cannot allocate memory\n");
        return 1;
    }
    
    fread(source, 1, file_size, file);
    source[file_size] = '\0';
    fclose(file);
    
    printf("=== Parsing file: %s ===\n\n", argv[1]);
    
    ASTNode* ast = parse_source(source, argv[1]);
    
    if (ast) {
        printf("Parsing successful!\n");
        printf("\nAST Structure:\n");
        printf("--------------\n");
        print_ast(ast, 0);
        
        free_ast_node(ast);
    } else {
        printf("Parsing failed!\n");
    }
    
    free(source);
    
    return 0;
}