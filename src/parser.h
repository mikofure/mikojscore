/*
 * MIT License
 *
 * Copyright (c) 2025 Ariz Kamizuki
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * MikoJSCore Parser
 * JavaScript parser for ES2020+ syntax - generates Abstract Syntax Tree (AST)
 */

#ifndef MIKOJS_PARSER_H
#define MIKOJS_PARSER_H

#include "mikojs_internal.h"
#include "lexer.h"

/* AST Node types */
typedef enum {
    /* Literals */
    AST_LITERAL_UNDEFINED,
    AST_LITERAL_NULL,
    AST_LITERAL_BOOLEAN,
    AST_LITERAL_NUMBER,
    AST_LITERAL_STRING,
    AST_LITERAL_BIGINT,
    AST_LITERAL_TEMPLATE,
    
    /* Identifiers */
    AST_IDENTIFIER,
    
    /* Expressions */
    AST_BINARY_EXPRESSION,
    AST_UNARY_EXPRESSION,
    AST_ASSIGNMENT_EXPRESSION,
    AST_UPDATE_EXPRESSION,
    AST_LOGICAL_EXPRESSION,
    AST_CONDITIONAL_EXPRESSION,
    AST_CALL_EXPRESSION,
    AST_MEMBER_EXPRESSION,
    AST_NEW_EXPRESSION,
    AST_THIS_EXPRESSION,
    AST_ARRAY_EXPRESSION,
    AST_OBJECT_EXPRESSION,
    AST_FUNCTION_EXPRESSION,
    AST_ARROW_FUNCTION_EXPRESSION,
    AST_SEQUENCE_EXPRESSION,
    AST_AWAIT_EXPRESSION,
    AST_YIELD_EXPRESSION,
    
    /* Statements */
    AST_EXPRESSION_STATEMENT,
    AST_BLOCK_STATEMENT,
    AST_EMPTY_STATEMENT,
    AST_DEBUGGER_STATEMENT,
    AST_WITH_STATEMENT,
    AST_RETURN_STATEMENT,
    AST_LABELED_STATEMENT,
    AST_BREAK_STATEMENT,
    AST_CONTINUE_STATEMENT,
    AST_IF_STATEMENT,
    AST_SWITCH_STATEMENT,
    AST_THROW_STATEMENT,
    AST_TRY_STATEMENT,
    AST_WHILE_STATEMENT,
    AST_DO_WHILE_STATEMENT,
    AST_FOR_STATEMENT,
    AST_FOR_IN_STATEMENT,
    AST_FOR_OF_STATEMENT,
    
    /* Declarations */
    AST_FUNCTION_DECLARATION,
    AST_VARIABLE_DECLARATION,
    AST_CLASS_DECLARATION,
    
    /* ES6+ */
    AST_IMPORT_DECLARATION,
    AST_EXPORT_DECLARATION,
    AST_SUPER,
    
    /* Program */
    AST_PROGRAM
} mjs_ast_type_t;

/* Forward declaration */
typedef struct mjs_ast_node mjs_ast_node_t;

/* AST Node structure */
struct mjs_ast_node {
    mjs_ast_type_t type;
    size_t line;
    size_t column;
    
    union {
        /* Literals */
        struct {
            mjs_value_t value;
        } literal;
        
        /* Identifier */
        struct {
            char* name;
        } identifier;
        
        /* Binary expression */
        struct {
            mjs_token_type_t operator;
            mjs_ast_node_t* left;
            mjs_ast_node_t* right;
        } binary;
        
        /* Unary expression */
        struct {
            mjs_token_type_t operator;
            mjs_ast_node_t* argument;
            bool prefix;
        } unary;
        
        /* Assignment expression */
        struct {
            mjs_token_type_t operator;
            mjs_ast_node_t* left;
            mjs_ast_node_t* right;
        } assignment;
        
        /* Update expression (++, --) */
        struct {
            mjs_token_type_t operator;
            mjs_ast_node_t* argument;
            bool prefix;
        } update;
        
        /* Logical expression */
        struct {
            mjs_token_type_t operator;
            mjs_ast_node_t* left;
            mjs_ast_node_t* right;
        } logical;
        
        /* Conditional expression (ternary) */
        struct {
            mjs_ast_node_t* test;
            mjs_ast_node_t* consequent;
            mjs_ast_node_t* alternate;
        } conditional;
        
        /* Call expression */
        struct {
            mjs_ast_node_t* callee;
            mjs_ast_node_t** arguments;
            size_t argument_count;
        } call;
        
        /* Member expression */
        struct {
            mjs_ast_node_t* object;
            mjs_ast_node_t* property;
            bool computed;
            bool optional;
        } member;
        
        /* Array expression */
        struct {
            mjs_ast_node_t** elements;
            size_t element_count;
        } array;
        
        /* Object expression */
        struct {
            struct {
                mjs_ast_node_t* key;
                mjs_ast_node_t* value;
                bool computed;
                bool method;
                bool shorthand;
            }* properties;
            size_t property_count;
        } object;
        
        /* Function expression/declaration */
        struct {
            char* name;
            mjs_ast_node_t** params;
            size_t param_count;
            mjs_ast_node_t* body;
            bool async;
            bool generator;
        } function;
        
        /* Block statement */
        struct {
            mjs_ast_node_t** statements;
            size_t statement_count;
        } block;
        
        /* If statement */
        struct {
            mjs_ast_node_t* test;
            mjs_ast_node_t* consequent;
            mjs_ast_node_t* alternate;
        } if_stmt;
        
        /* While statement */
        struct {
            mjs_ast_node_t* test;
            mjs_ast_node_t* body;
        } while_stmt;
        
        /* For statement */
        struct {
            mjs_ast_node_t* init;
            mjs_ast_node_t* test;
            mjs_ast_node_t* update;
            mjs_ast_node_t* body;
        } for_stmt;
        
        /* Variable declaration */
        struct {
            enum {
                VAR_DECLARATION_VAR,
                VAR_DECLARATION_LET,
                VAR_DECLARATION_CONST
            } kind;
            struct {
                mjs_ast_node_t* id;
                mjs_ast_node_t* init;
            }* declarations;
            size_t declaration_count;
        } var_decl;
        
        /* Return statement */
        struct {
            mjs_ast_node_t* argument;
        } return_stmt;
        
        /* Expression statement */
        struct {
            mjs_ast_node_t* expression;
        } expr_stmt;
        
        /* Program */
        struct {
            mjs_ast_node_t** body;
            size_t statement_count;
            bool strict_mode;
        } program;
        
    } u;
};

/* Parser structure */
struct mjs_parser {
    mjs_lexer_t* lexer;
    mjs_token_t current_token;
    mjs_token_t previous_token;
    bool has_error;
    char* error_message;
    mjs_context_t* context;
};

/* Parser functions */
mjs_parser_t* mjs_parser_new(mjs_context_t* ctx, const char* source, size_t length);
void mjs_parser_free(mjs_parser_t* parser);

mjs_ast_node_t* mjs_parser_parse(mjs_parser_t* parser);
mjs_ast_node_t* mjs_parser_parse_expression(mjs_parser_t* parser);
mjs_ast_node_t* mjs_parser_parse_statement(mjs_parser_t* parser);

bool mjs_parser_has_error(mjs_parser_t* parser);
const char* mjs_parser_get_error(mjs_parser_t* parser);

/* AST utilities */
mjs_ast_node_t* mjs_ast_node_new(mjs_context_t* ctx, mjs_ast_type_t type);
void mjs_ast_node_free(mjs_ast_node_t* node);
void mjs_ast_free(mjs_ast_node_t* node);
void mjs_ast_dump(mjs_ast_node_t* node, int indent);

/* AST node creation helpers */
mjs_ast_node_t* mjs_ast_literal_new(mjs_context_t* ctx, mjs_value_t value);
mjs_ast_node_t* mjs_ast_identifier_new(mjs_context_t* ctx, const char* name);
mjs_ast_node_t* mjs_ast_binary_new(mjs_context_t* ctx, mjs_token_type_t op, mjs_ast_node_t* left, mjs_ast_node_t* right);
mjs_ast_node_t* mjs_ast_unary_new(mjs_context_t* ctx, mjs_token_type_t op, mjs_ast_node_t* argument, bool prefix);
mjs_ast_node_t* mjs_ast_call_new(mjs_context_t* ctx, mjs_ast_node_t* callee, mjs_ast_node_t** args, size_t arg_count);

#endif /* MIKOJS_PARSER_H */