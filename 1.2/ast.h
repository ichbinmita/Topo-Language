#ifndef AST_H
#define AST_H

#include <stdbool.h>

// ================ AST NODE TYPES ================
typedef enum {
    // Statements
    NODE_PROGRAM,
    NODE_BLOCK,
    NODE_VAR_DECL,
    NODE_CONST_DECL,
    NODE_FUNC_DECL,
    NODE_IF_STMT,
    NODE_ELIF_STMT,
    NODE_ELSE_STMT,
    NODE_WHILE_STMT,
    NODE_FOR_STMT,
    NODE_RETURN_STMT,
    NODE_BREAK_STMT,
    NODE_CONTINUE_STMT,
    NODE_EXPR_STMT,
    NODE_FROM_IMPORT,
    
    // Expressions
    NODE_BINARY_EXPR,
    NODE_UNARY_EXPR,
    NODE_LITERAL,
    NODE_IDENTIFIER,
    NODE_ASSIGNMENT,
    NODE_CALL_EXPR,
    NODE_ARRAY_LITERAL,
    NODE_DICT_LITERAL,
    NODE_MEMBER_ACCESS,
    NODE_INDEX_ACCESS,
    NODE_RANGE_EXPR,
    
    // Types
    NODE_TYPE_ANNOTATION
} NodeType;

// ================ DATA TYPES ================
typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_STRING,
    TYPE_BOOL,
    TYPE_ARRAY,
    TYPE_DICT,
    TYPE_FUNCTION,
    TYPE_NULL,
    TYPE_ANY
} DataType;

// ================ LITERAL VALUE ================
typedef union {
    long int_val;
    double float_val;
    char* string_val;
    bool bool_val;
} LiteralValue;

// ================ AST NODE STRUCTURE ================
typedef struct ASTNode ASTNode;
typedef struct FunctionParam FunctionParam;

struct FunctionParam {
    char* name;
    DataType type;
    FunctionParam* next;
};

struct ASTNode {
    NodeType type;
    int line;
    int column;
    
    // Common fields
    char* name;
    ASTNode* next; // For linked lists (statements, parameters)
    
    // Statement fields
    union {
        // Variable/Constant declaration
        struct {
            DataType data_type;
            ASTNode* value;
            bool is_const;
        } decl;
        
        // Function declaration
        struct {
            FunctionParam* params;
            ASTNode* body;
            DataType return_type;
        } func;
        
        // Control flow
        struct {
            ASTNode* condition;
            ASTNode* then_branch;
            ASTNode* else_branch;
            ASTNode* elif_branches; // Linked list of ELIF nodes
        } flow;
        
        // Loops
        struct {
            ASTNode* init;
            ASTNode* condition;
            ASTNode* update;
            ASTNode* body;
            char* iterator;
            ASTNode* iterable;
        } loop;
        
        // Return statement
        struct {
            ASTNode* value;
        } ret;
        
        // Block
        struct {
            ASTNode* statements;
        } block;
        
        // From import
        struct {
            char* module_name;
            char** imports;
            int import_count;
            bool import_all;
        } import;
        
        // Expression fields
        union {
            // Binary expression
            struct {
                char* op;
                ASTNode* left;
                ASTNode* right;
            } binary;
            
            // Unary expression
            struct {
                char* op;
                ASTNode* operand;
            } unary;
            
            // Literal
            struct {
                LiteralValue value;
                DataType data_type;
            } literal;
            
            // Identifier
            struct {
                char* identifier;
            } identifier;
            
            // Assignment
            struct {
                ASTNode* target;
                ASTNode* value;
            } assign;
            
            // Function call
            struct {
                ASTNode* callee;
                ASTNode* arguments;
                int arg_count;
            } call;
            
            // Array literal
            struct {
                ASTNode* elements;
                int element_count;
            } array;
            
            // Dictionary literal
            struct {
                char** keys;
                ASTNode* values;
                int pair_count;
            } dict;
            
            // Member access (obj.property)
            struct {
                ASTNode* object;
                char* member;
            } member;
            
            // Index access (array[index])
            struct {
                ASTNode* array;
                ASTNode* index;
            } index;
            
            // Range expression
            struct {
                ASTNode* start;
                ASTNode* end;
                ASTNode* step;
            } range;
        } expr;
    };
};

// ================ AST CREATION FUNCTIONS ================
ASTNode* create_program_node(ASTNode* statements);
ASTNode* create_block_node(ASTNode* statements, int line, int column);
ASTNode* create_var_decl_node(char* name, ASTNode* value, bool is_const, int line, int column);
ASTNode* create_func_decl_node(char* name, FunctionParam* params, ASTNode* body, DataType return_type, int line, int column);
ASTNode* create_if_node(ASTNode* condition, ASTNode* then_branch, ASTNode* else_branch, int line, int column);
ASTNode* create_elif_node(ASTNode* condition, ASTNode* then_branch, int line, int column);
ASTNode* create_while_node(ASTNode* condition, ASTNode* body, int line, int column);
ASTNode* create_for_node(char* iterator, ASTNode* iterable, ASTNode* body, int line, int column);
ASTNode* create_return_node(ASTNode* value, int line, int column);
ASTNode* create_break_node(int line, int column);
ASTNode* create_continue_node(int line, int column);
ASTNode* create_expr_stmt_node(ASTNode* expr, int line, int column);
ASTNode* create_from_import_node(char* module_name, char** imports, int import_count, bool import_all, int line, int column);

// Expression nodes
ASTNode* create_binary_expr_node(char* op, ASTNode* left, ASTNode* right, int line, int column);
ASTNode* create_unary_expr_node(char* op, ASTNode* operand, int line, int column);
ASTNode* create_literal_node_int(long value, int line, int column);
ASTNode* create_literal_node_float(double value, int line, int column);
ASTNode* create_literal_node_string(char* value, int line, int column);
ASTNode* create_literal_node_bool(bool value, int line, int column);
ASTNode* create_literal_node_null(int line, int column);
ASTNode* create_identifier_node(char* name, int line, int column);
ASTNode* create_assignment_node(ASTNode* target, ASTNode* value, int line, int column);
ASTNode* create_call_expr_node(ASTNode* callee, ASTNode* arguments, int arg_count, int line, int column);
ASTNode* create_array_literal_node(ASTNode* elements, int element_count, int line, int column);
ASTNode* create_dict_literal_node(char** keys, ASTNode* values, int pair_count, int line, int column);
ASTNode* create_member_access_node(ASTNode* object, char* member, int line, int column);
ASTNode* create_index_access_node(ASTNode* array, ASTNode* index, int line, int column);
ASTNode* create_range_node(ASTNode* start, ASTNode* end, ASTNode* step, int line, int column);

// Utility functions
FunctionParam* create_function_param(char* name, DataType type);
void add_statement_to_block(ASTNode* block, ASTNode* statement);
void add_elif_branch(ASTNode* if_node, ASTNode* elif_node);
void add_next_statement(ASTNode* current, ASTNode* next);
void add_argument_to_call(ASTNode* call_node, ASTNode* argument);
void add_element_to_array(ASTNode* array_node, ASTNode* element);
void add_pair_to_dict(ASTNode* dict_node, char* key, ASTNode* value);

// Memory management
void free_ast_node(ASTNode* node);
void free_function_params(FunctionParam* params);

// Debug/Print functions
void print_ast(ASTNode* node, int indent);
const char* node_type_to_string(NodeType type);
const char* data_type_to_string(DataType type);

#endif // AST_H