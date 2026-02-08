/**
 * AST implementation for Topo Programming Language
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "ast.h"

// ================ AST CREATION FUNCTIONS ================

ASTNode* create_program_node(ASTNode* statements) {
    ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = NODE_PROGRAM;
    node->block.statements = statements;
    
    return node;
}

ASTNode* create_block_node(ASTNode* statements, int line, int column) {
    ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = NODE_BLOCK;
    node->line = line;
    node->column = column;
    node->block.statements = statements;
    
    return node;
}

ASTNode* create_var_decl_node(char* name, ASTNode* value, bool is_const, int line, int column) {
    ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = is_const ? NODE_CONST_DECL : NODE_VAR_DECL;
    node->line = line;
    node->column = column;
    node->name = name ? strdup(name) : NULL;
    node->decl.value = value;
    node->decl.is_const = is_const;
    node->decl.data_type = TYPE_ANY;
    
    return node;
}

ASTNode* create_func_decl_node(char* name, FunctionParam* params, ASTNode* body, DataType return_type, int line, int column) {
    ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = NODE_FUNC_DECL;
    node->line = line;
    node->column = column;
    node->name = name ? strdup(name) : NULL;
    node->func.params = params;
    node->func.body = body;
    node->func.return_type = return_type;
    
    return node;
}

ASTNode* create_if_node(ASTNode* condition, ASTNode* then_branch, ASTNode* else_branch, int line, int column) {
    ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = NODE_IF_STMT;
    node->line = line;
    node->column = column;
    node->flow.condition = condition;
    node->flow.then_branch = then_branch;
    node->flow.else_branch = else_branch;
    node->flow.elif_branches = NULL;
    
    return node;
}

ASTNode* create_elif_node(ASTNode* condition, ASTNode* then_branch, int line, int column) {
    ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = NODE_ELIF_STMT;
    node->line = line;
    node->column = column;
    node->flow.condition = condition;
    node->flow.then_branch = then_branch;
    
    return node;
}

ASTNode* create_while_node(ASTNode* condition, ASTNode* body, int line, int column) {
    ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = NODE_WHILE_STMT;
    node->line = line;
    node->column = column;
    node->flow.condition = condition;
    node->flow.then_branch = body;
    
    return node;
}

ASTNode* create_for_node(char* iterator, ASTNode* iterable, ASTNode* body, int line, int column) {
    ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = NODE_FOR_STMT;
    node->line = line;
    node->column = column;
    node->name = iterator ? strdup(iterator) : NULL;
    node->loop.iterable = iterable;
    node->loop.body = body;
    
    return node;
}

ASTNode* create_return_node(ASTNode* value, int line, int column) {
    ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = NODE_RETURN_STMT;
    node->line = line;
    node->column = column;
    node->ret.value = value;
    
    return node;
}

ASTNode* create_break_node(int line, int column) {
    ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = NODE_BREAK_STMT;
    node->line = line;
    node->column = column;
    
    return node;
}

ASTNode* create_continue_node(int line, int column) {
    ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = NODE_CONTINUE_STMT;
    node->line = line;
    node->column = column;
    
    return node;
}

ASTNode* create_expr_stmt_node(ASTNode* expr, int line, int column) {
    ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = NODE_EXPR_STMT;
    node->line = line;
    node->column = column;
    node->expr.binary.left = expr; // Reusing binary.left for expression
    
    return node;
}

ASTNode* create_from_import_node(char* module_name, char** imports, int import_count, bool import_all, int line, int column) {
    ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = NODE_FROM_IMPORT;
    node->line = line;
    node->column = column;
    node->name = module_name ? strdup(module_name) : NULL;
    node->import.imports = imports;
    node->import.import_count = import_count;
    node->import.import_all = import_all;
    
    return node;
}

// Expression nodes
ASTNode* create_binary_expr_node(char* op, ASTNode* left, ASTNode* right, int line, int column) {
    ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = NODE_BINARY_EXPR;
    node->line = line;
    node->column = column;
    node->expr.binary.op = op ? strdup(op) : NULL;
    node->expr.binary.left = left;
    node->expr.binary.right = right;
    
    return node;
}

ASTNode* create_unary_expr_node(char* op, ASTNode* operand, int line, int column) {
    ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = NODE_UNARY_EXPR;
    node->line = line;
    node->column = column;
    node->expr.unary.op = op ? strdup(op) : NULL;
    node->expr.unary.operand = operand;
    
    return node;
}

ASTNode* create_literal_node_int(long value, int line, int column) {
    ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = NODE_LITERAL;
    node->line = line;
    node->column = column;
    node->expr.literal.value.int_val = value;
    node->expr.literal.data_type = TYPE_INT;
    
    return node;
}

ASTNode* create_literal_node_float(double value, int line, int column) {
    ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = NODE_LITERAL;
    node->line = line;
    node->column = column;
    node->expr.literal.value.float_val = value;
    node->expr.literal.data_type = TYPE_FLOAT;
    
    return node;
}

ASTNode* create_literal_node_string(char* value, int line, int column) {
    ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = NODE_LITERAL;
    node->line = line;
    node->column = column;
    node->expr.literal.value.string_val = value ? strdup(value) : NULL;
    node->expr.literal.data_type = TYPE_STRING;
    
    return node;
}

ASTNode* create_literal_node_bool(bool value, int line, int column) {
    ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = NODE_LITERAL;
    node->line = line;
    node->column = column;
    node->expr.literal.value.bool_val = value;
    node->expr.literal.data_type = TYPE_BOOL;
    
    return node;
}

ASTNode* create_literal_node_null(int line, int column) {
    ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = NODE_LITERAL;
    node->line = line;
    node->column = column;
    node->expr.literal.data_type = TYPE_NULL;
    
    return node;
}

ASTNode* create_identifier_node(char* name, int line, int column) {
    ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = NODE_IDENTIFIER;
    node->line = line;
    node->column = column;
    node->expr.identifier.identifier = name ? strdup(name) : NULL;
    
    return node;
}

ASTNode* create_assignment_node(ASTNode* target, ASTNode* value, int line, int column) {
    ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = NODE_ASSIGNMENT;
    node->line = line;
    node->column = column;
    node->expr.assign.target = target;
    node->expr.assign.value = value;
    
    return node;
}

ASTNode* create_call_expr_node(ASTNode* callee, ASTNode* arguments, int arg_count, int line, int column) {
    ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = NODE_CALL_EXPR;
    node->line = line;
    node->column = column;
    node->expr.call.callee = callee;
    node->expr.call.arguments = arguments;
    node->expr.call.arg_count = arg_count;
    
    return node;
}

ASTNode* create_array_literal_node(ASTNode* elements, int element_count, int line, int column) {
    ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = NODE_ARRAY_LITERAL;
    node->line = line;
    node->column = column;
    node->expr.array.elements = elements;
    node->expr.array.element_count = element_count;
    
    return node;
}

ASTNode* create_dict_literal_node(char** keys, ASTNode* values, int pair_count, int line, int column) {
    ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = NODE_DICT_LITERAL;
    node->line = line;
    node->column = column;
    node->expr.dict.keys = keys;
    node->expr.dict.values = values;
    node->expr.dict.pair_count = pair_count;
    
    return node;
}

ASTNode* create_member_access_node(ASTNode* object, char* member, int line, int column) {
    ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = NODE_MEMBER_ACCESS;
    node->line = line;
    node->column = column;
    node->expr.member.object = object;
    node->expr.member.member = member ? strdup(member) : NULL;
    
    return node;
}

ASTNode* create_index_access_node(ASTNode* array, ASTNode* index, int line, int column) {
    ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = NODE_INDEX_ACCESS;
    node->line = line;
    node->column = column;
    node->expr.index.array = array;
    node->expr.index.index = index;
    
    return node;
}

ASTNode* create_range_node(ASTNode* start, ASTNode* end, ASTNode* step, int line, int column) {
    ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
    if (!node) return NULL;
    
    node->type = NODE_RANGE_EXPR;
    node->line = line;
    node->column = column;
    node->expr.range.start = start;
    node->expr.range.end = end;
    node->expr.range.step = step;
    
    return node;
}

// ================ UTILITY FUNCTIONS ================

FunctionParam* create_function_param(char* name, DataType type) {
    FunctionParam* param = (FunctionParam*)calloc(1, sizeof(FunctionParam));
    if (!param) return NULL;
    
    param->name = name ? strdup(name) : NULL;
    param->type = type;
    param->next = NULL;
    
    return param;
}

void add_statement_to_block(ASTNode* block, ASTNode* statement) {
    if (!block || block->type != NODE_BLOCK || !statement) return;
    
    if (!block->block.statements) {
        block->block.statements = statement;
    } else {
        ASTNode* current = block->block.statements;
        while (current->next) {
            current = current->next;
        }
        current->next = statement;
    }
}

void add_elif_branch(ASTNode* if_node, ASTNode* elif_node) {
    if (!if_node || if_node->type != NODE_IF_STMT || !elif_node || elif_node->type != NODE_ELIF_STMT) return;
    
    if (!if_node->flow.elif_branches) {
        if_node->flow.elif_branches = elif_node;
    } else {
        ASTNode* current = if_node->flow.elif_branches;
        while (current->next) {
            current = current->next;
        }
        current->next = elif_node;
    }
}

void add_next_statement(ASTNode* current, ASTNode* next) {
    if (!current || !next) return;
    current->next = next;
}

void add_argument_to_call(ASTNode* call_node, ASTNode* argument) {
    if (!call_node || call_node->type != NODE_CALL_EXPR || !argument) return;
    
    if (!call_node->expr.call.arguments) {
        call_node->expr.call.arguments = argument;
    } else {
        ASTNode* current = call_node->expr.call.arguments;
        while (current->next) {
            current = current->next;
        }
        current->next = argument;
    }
    call_node->expr.call.arg_count++;
}

void add_element_to_array(ASTNode* array_node, ASTNode* element) {
    if (!array_node || array_node->type != NODE_ARRAY_LITERAL || !element) return;
    
    if (!array_node->expr.array.elements) {
        array_node->expr.array.elements = element;
    } else {
        ASTNode* current = array_node->expr.array.elements;
        while (current->next) {
            current = current->next;
        }
        current->next = element;
    }
    array_node->expr.array.element_count++;
}

void add_pair_to_dict(ASTNode* dict_node, char* key, ASTNode* value) {
    if (!dict_node || dict_node->type != NODE_DICT_LITERAL || !key || !value) return;
    
    int new_count = dict_node->expr.dict.pair_count + 1;
    
    // Reallocate keys array
    char** new_keys = (char**)realloc(dict_node->expr.dict.keys, new_count * sizeof(char*));
    if (!new_keys) return;
    
    dict_node->expr.dict.keys = new_keys;
    dict_node->expr.dict.keys[new_count - 1] = strdup(key);
    
    // Add value to linked list
    if (!dict_node->expr.dict.values) {
        dict_node->expr.dict.values = value;
    } else {
        ASTNode* current = dict_node->expr.dict.values;
        while (current->next) {
            current = current->next;
        }
        current->next = value;
    }
    
    dict_node->expr.dict.pair_count = new_count;
}

// ================ MEMORY MANAGEMENT ================

void free_function_params(FunctionParam* params) {
    while (params) {
        FunctionParam* next = params->next;
        if (params->name) free(params->name);
        free(params);
        params = next;
    }
}

void free_ast_node(ASTNode* node) {
    if (!node) return;
    
    // Free name if present
    if (node->name) {
        free(node->name);
    }
    
    // Free based on node type
    switch (node->type) {
        case NODE_PROGRAM:
        case NODE_BLOCK:
            if (node->block.statements) {
                ASTNode* stmt = node->block.statements;
                while (stmt) {
                    ASTNode* next = stmt->next;
                    free_ast_node(stmt);
                    stmt = next;
                }
            }
            break;
            
        case NODE_VAR_DECL:
        case NODE_CONST_DECL:
            if (node->decl.value) free_ast_node(node->decl.value);
            break;
            
        case NODE_FUNC_DECL:
            free_function_params(node->func.params);
            if (node->func.body) free_ast_node(node->func.body);
            break;
            
        case NODE_IF_STMT:
            if (node->flow.condition) free_ast_node(node->flow.condition);
            if (node->flow.then_branch) free_ast_node(node->flow.then_branch);
            if (node->flow.else_branch) free_ast_node(node->flow.else_branch);
            if (node->flow.elif_branches) free_ast_node(node->flow.elif_branches);
            break;
            
        case NODE_ELIF_STMT:
            if (node->flow.condition) free_ast_node(node->flow.condition);
            if (node->flow.then_branch) free_ast_node(node->flow.then_branch);
            break;
            
        case NODE_WHILE_STMT:
            if (node->flow.condition) free_ast_node(node->flow.condition);
            if (node->flow.then_branch) free_ast_node(node->flow.then_branch);
            break;
            
        case NODE_FOR_STMT:
            if (node->loop.iterable) free_ast_node(node->loop.iterable);
            if (node->loop.body) free_ast_node(node->loop.body);
            break;
            
        case NODE_RETURN_STMT:
            if (node->ret.value) free_ast_node(node->ret.value);
            break;
            
        case NODE_EXPR_STMT:
            if (node->expr.binary.left) free_ast_node(node->expr.binary.left);
            break;
            
        case NODE_FROM_IMPORT:
            if (node->import.imports) {
                for (int i = 0; i < node->import.import_count; i++) {
                    if (node->import.imports[i]) free(node->import.imports[i]);
                }
                free(node->import.imports);
            }
            break;
            
        case NODE_BINARY_EXPR:
            if (node->expr.binary.op) free(node->expr.binary.op);
            if (node->expr.binary.left) free_ast_node(node->expr.binary.left);
            if (node->expr.binary.right) free_ast_node(node->expr.binary.right);
            break;
            
        case NODE_UNARY_EXPR:
            if (node->expr.unary.op) free(node->expr.unary.op);
            if (node->expr.unary.operand) free_ast_node(node->expr.unary.operand);
            break;
            
        case NODE_LITERAL:
            if (node->expr.literal.data_type == TYPE_STRING && 
                node->expr.literal.value.string_val) {
                free(node->expr.literal.value.string_val);
            }
            break;
            
        case NODE_IDENTIFIER:
            if (node->expr.identifier.identifier) free(node->expr.identifier.identifier);
            break;
            
        case NODE_ASSIGNMENT:
            if (node->expr.assign.target) free_ast_node(node->expr.assign.target);
            if (node->expr.assign.value) free_ast_node(node->expr.assign.value);
            break;
            
        case NODE_CALL_EXPR:
            if (node->expr.call.callee) free_ast_node(node->expr.call.callee);
            if (node->expr.call.arguments) {
                ASTNode* arg = node->expr.call.arguments;
                while (arg) {
                    ASTNode* next = arg->next;
                    free_ast_node(arg);
                    arg = next;
                }
            }
            break;
            
        case NODE_ARRAY_LITERAL:
            if (node->expr.array.elements) {
                ASTNode* elem = node->expr.array.elements;
                while (elem) {
                    ASTNode* next = elem->next;
                    free_ast_node(elem);
                    elem = next;
                }
            }
            break;
            
        case NODE_DICT_LITERAL:
            if (node->expr.dict.keys) {
                for (int i = 0; i < node->expr.dict.pair_count; i++) {
                    if (node->expr.dict.keys[i]) free(node->expr.dict.keys[i]);
                }
                free(node->expr.dict.keys);
            }
            if (node->expr.dict.values) {
                ASTNode* val = node->expr.dict.values;
                while (val) {
                    ASTNode* next = val->next;
                    free_ast_node(val);
                    val = next;
                }
            }
            break;
            
        case NODE_MEMBER_ACCESS:
            if (node->expr.member.object) free_ast_node(node->expr.member.object);
            if (node->expr.member.member) free(node->expr.member.member);
            break;
            
        case NODE_INDEX_ACCESS:
            if (node->expr.index.array) free_ast_node(node->expr.index.array);
            if (node->expr.index.index) free_ast_node(node->expr.index.index);
            break;
            
        case NODE_RANGE_EXPR:
            if (node->expr.range.start) free_ast_node(node->expr.range.start);
            if (node->expr.range.end) free_ast_node(node->expr.range.end);
            if (node->expr.range.step) free_ast_node(node->expr.range.step);
            break;
            
        default:
            break;
    }
    
    // Free next node in linked list
    if (node->next) {
        free_ast_node(node->next);
    }
    
    free(node);
}

// ================ DEBUG/PRINT FUNCTIONS ================

const char* node_type_to_string(NodeType type) {
    switch (type) {
        case NODE_PROGRAM: return "PROGRAM";
        case NODE_BLOCK: return "BLOCK";
        case NODE_VAR_DECL: return "VAR_DECL";
        case NODE_CONST_DECL: return "CONST_DECL";
        case NODE_FUNC_DECL: return "FUNC_DECL";
        case NODE_IF_STMT: return "IF_STMT";
        case NODE_ELIF_STMT: return "ELIF_STMT";
        case NODE_ELSE_STMT: return "ELSE_STMT";
        case NODE_WHILE_STMT: return "WHILE_STMT";
        case NODE_FOR_STMT: return "FOR_STMT";
        case NODE_RETURN_STMT: return "RETURN_STMT";
        case NODE_BREAK_STMT: return "BREAK_STMT";
        case NODE_CONTINUE_STMT: return "CONTINUE_STMT";
        case NODE_EXPR_STMT: return "EXPR_STMT";
        case NODE_FROM_IMPORT: return "FROM_IMPORT";
        case NODE_BINARY_EXPR: return "BINARY_EXPR";
        case NODE_UNARY_EXPR: return "UNARY_EXPR";
        case NODE_LITERAL: return "LITERAL";
        case NODE_IDENTIFIER: return "IDENTIFIER";
        case NODE_ASSIGNMENT: return "ASSIGNMENT";
        case NODE_CALL_EXPR: return "CALL_EXPR";
        case NODE_ARRAY_LITERAL: return "ARRAY_LITERAL";
        case NODE_DICT_LITERAL: return "DICT_LITERAL";
        case NODE_MEMBER_ACCESS: return "MEMBER_ACCESS";
        case NODE_INDEX_ACCESS: return "INDEX_ACCESS";
        case NODE_RANGE_EXPR: return "RANGE_EXPR";
        case NODE_TYPE_ANNOTATION: return "TYPE_ANNOTATION";
        default: return "UNKNOWN";
    }
}

const char* data_type_to_string(DataType type) {
    switch (type) {
        case TYPE_INT: return "int";
        case TYPE_FLOAT: return "float";
        case TYPE_STRING: return "string";
        case TYPE_BOOL: return "bool";
        case TYPE_ARRAY: return "array";
        case TYPE_DICT: return "dict";
        case TYPE_FUNCTION: return "function";
        case TYPE_NULL: return "null";
        case TYPE_ANY: return "any";
        default: return "unknown";
    }
}

void print_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

void print_ast(ASTNode* node, int indent) {
    if (!node) return;
    
    print_indent(indent);
    printf("%s", node_type_to_string(node->type));
    
    // Print node-specific information
    switch (node->type) {
        case NODE_PROGRAM:
            printf(":\n");
            if (node->block.statements) {
                print_ast(node->block.statements, indent + 1);
            }
            break;
            
        case NODE_BLOCK:
            printf(" (%d statements):\n", node->block.statements ? 1 : 0);
            if (node->block.statements) {
                ASTNode* stmt = node->block.statements;
                while (stmt) {
                    print_ast(stmt, indent + 1);
                    stmt = stmt->next;
                }
            }
            break;
            
        case NODE_VAR_DECL:
        case NODE_CONST_DECL:
            printf(" %s", node->name ? node->name : "<unnamed>");
            if (node->decl.data_type != TYPE_ANY) {
                printf(": %s", data_type_to_string(node->decl.data_type));
            }
            printf("\n");
            if (node->decl.value) {
                print_indent(indent + 1);
                printf("value:\n");
                print_ast(node->decl.value, indent + 2);
            }
            break;
            
        case NODE_FUNC_DECL:
            printf(" %s", node->name ? node->name : "<unnamed>");
            printf(" -> %s\n", data_type_to_string(node->func.return_type));
            if (node->func.body) {
                print_indent(indent + 1);
                printf("body:\n");
                print_ast(node->func.body, indent + 2);
            }
            break;
            
        case NODE_IF_STMT:
            printf(":\n");
            print_indent(indent + 1);
            printf("condition:\n");
            if (node->flow.condition) print_ast(node->flow.condition, indent + 2);
            print_indent(indent + 1);
            printf("then:\n");
            if (node->flow.then_branch) print_ast(node->flow.then_branch, indent + 2);
            if (node->flow.else_branch) {
                print_indent(indent + 1);
                printf("else:\n");
                print_ast(node->flow.else_branch, indent + 2);
            }
            break;
            
        case NODE_WHILE_STMT:
            printf(":\n");
            print_indent(indent + 1);
            printf("condition:\n");
            if (node->flow.condition) print_ast(node->flow.condition, indent + 2);
            print_indent(indent + 1);
            printf("body:\n");
            if (node->flow.then_branch) print_ast(node->flow.then_branch, indent + 2);
            break;
            
        case NODE_FOR_STMT:
            printf(" %s in:\n", node->name ? node->name : "<iterator>");
            print_indent(indent + 1);
            printf("iterable:\n");
            if (node->loop.iterable) print_ast(node->loop.iterable, indent + 2);
            print_indent(indent + 1);
            printf("body:\n");
            if (node->loop.body) print_ast(node->loop.body, indent + 2);
            break;
            
        case NODE_RETURN_STMT:
            printf("\n");
            if (node->ret.value) {
                print_indent(indent + 1);
                printf("value:\n");
                print_ast(node->ret.value, indent + 2);
            }
            break;
            
        case NODE_EXPR_STMT:
            printf(":\n");
            if (node->expr.binary.left) print_ast(node->expr.binary.left, indent + 1);
            break;
            
        case NODE_FROM_IMPORT:
            printf(" from %s", node->name ? node->name : "<module>");
            if (node->import.import_all) {
                printf(" import *\n");
            } else {
                printf(" import:\n");
                for (int i = 0; i < node->import.import_count; i++) {
                    print_indent(indent + 1);
                    printf("%s\n", node->import.imports[i]);
                }
            }
            break;
            
        case NODE_BINARY_EXPR:
            printf(" %s\n", node->expr.binary.op ? node->expr.binary.op : "<op>");
            print_indent(indent + 1);
            printf("left:\n");
            if (node->expr.binary.left) print_ast(node->expr.binary.left, indent + 2);
            print_indent(indent + 1);
            printf("right:\n");
            if (node->expr.binary.right) print_ast(node->expr.binary.right, indent + 2);
            break;
            
        case NODE_UNARY_EXPR:
            printf(" %s\n", node->expr.unary.op ? node->expr.unary.op : "<op>");
            print_indent(indent + 1);
            printf("operand:\n");
            if (node->expr.unary.operand) print_ast(node->expr.unary.operand, indent + 2);
            break;
            
        case NODE_LITERAL:
            switch (node->expr.literal.data_type) {
                case TYPE_INT:
                    printf(" int: %ld\n", node->expr.literal.value.int_val);
                    break;
                case TYPE_FLOAT:
                    printf(" float: %g\n", node->expr.literal.value.float_val);
                    break;
                case TYPE_STRING:
                    printf(" string: \"%s\"\n", node->expr.literal.value.string_val);
                    break;
                case TYPE_BOOL:
                    printf(" bool: %s\n", node->expr.literal.value.bool_val ? "true" : "false");
                    break;
                case TYPE_NULL:
                    printf(" null\n");
                    break;
                default:
                    printf(" <unknown>\n");
                    break;
            }
            break;
            
        case NODE_IDENTIFIER:
            printf(" %s\n", node->expr.identifier.identifier ? node->expr.identifier.identifier : "<unnamed>");
            break;
            
        case NODE_ASSIGNMENT:
            printf(":\n");
            print_indent(indent + 1);
            printf("target:\n");
            if (node->expr.assign.target) print_ast(node->expr.assign.target, indent + 2);
            print_indent(indent + 1);
            printf("value:\n");
            if (node->expr.assign.value) print_ast(node->expr.assign.value, indent + 2);
            break;
            
        case NODE_CALL_EXPR:
            printf(" (%d args):\n", node->expr.call.arg_count);
            print_indent(indent + 1);
            printf("callee:\n");
            if (node->expr.call.callee) print_ast(node->expr.call.callee, indent + 2);
            if (node->expr.call.arg_count > 0) {
                print_indent(indent + 1);
                printf("arguments:\n");
                ASTNode* arg = node->expr.call.arguments;
                while (arg) {
                    print_ast(arg, indent + 2);
                    arg = arg->next;
                }
            }
            break;
            
        case NODE_ARRAY_LITERAL:
            printf(" (%d elements):\n", node->expr.array.element_count);
            if (node->expr.array.element_count > 0) {
                ASTNode* elem = node->expr.array.elements;
                while (elem) {
                    print_ast(elem, indent + 1);
                    elem = elem->next;
                }
            }
            break;
            
        default:
            printf("\n");
            break;
    }
    
    // Print next node in linked list
    if (node->next) {
        print_ast(node->next, indent);
    }
}