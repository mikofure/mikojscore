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
 * MikoJSCore Parser Tests
 */

#include "../src/parser.h"
#include "../include/mikojs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_ASSERT(condition, message) do { \
    if (condition) { \
        printf("  ✓ %s\n", message); \
    } else { \
        printf("  ✗ %s\n", message); \
        return 1; \
    } \
} while(0)

#define TEST_SUITE_BEGIN(name) do { \
    printf("\n=== %s ===\n", name); \
} while(0)

static int test_parser_basic_expressions(void) {
    TEST_SUITE_BEGIN("Basic Expression Parsing");
    
    const char* source = "2 + 3 * 4";
    mjs_lexer_t* lexer = mjs_lexer_new(source);
    mjs_parser_t* parser = mjs_parser_new(lexer);
    
    TEST_ASSERT(parser != NULL, "Parser creation");
    
    mjs_ast_node_t* ast = mjs_parser_parse_expression(parser);
    TEST_ASSERT(ast != NULL, "Expression parsing");
    TEST_ASSERT(ast->type == AST_BINARY_EXPRESSION, "Binary expression AST type");
    
    mjs_ast_node_free(ast);
    mjs_parser_free(parser);
    
    return 0;
}

static int test_parser_statements(void) {
    TEST_SUITE_BEGIN("Statement Parsing");
    
    const char* source = "var x = 42;";
    mjs_lexer_t* lexer = mjs_lexer_new(source);
    mjs_parser_t* parser = mjs_parser_new(lexer);
    
    TEST_ASSERT(parser != NULL, "Parser creation");
    
    mjs_ast_node_t* ast = mjs_parser_parse_statement(parser);
    TEST_ASSERT(ast != NULL, "Statement parsing");
    TEST_ASSERT(ast->type == AST_VARIABLE_DECLARATION, "Variable declaration AST type");
    
    mjs_ast_node_free(ast);
    mjs_parser_free(parser);
    
    return 0;
}

static int test_parser_functions(void) {
    TEST_SUITE_BEGIN("Function Parsing");
    
    const char* source = "function add(a, b) { return a + b; }";
    mjs_lexer_t* lexer = mjs_lexer_new(source);
    mjs_parser_t* parser = mjs_parser_new(lexer);
    
    TEST_ASSERT(parser != NULL, "Parser creation");
    
    mjs_ast_node_t* ast = mjs_parser_parse_statement(parser);
    TEST_ASSERT(ast != NULL, "Function parsing");
    TEST_ASSERT(ast->type == AST_FUNCTION_DECLARATION, "Function declaration AST type");
    
    mjs_ast_node_free(ast);
    mjs_parser_free(parser);
    
    return 0;
}

int test_parser_run(void) {
    printf("\n=== Running Parser Tests ===\n");
    
    int result = 0;
    
    result |= test_parser_basic_expressions();
    result |= test_parser_statements();
    result |= test_parser_functions();
    
    if (result == 0) {
        printf("\n✅ All parser tests passed!\n");
    } else {
        printf("\n❌ Some parser tests failed.\n");
    }
    
    return result;
}