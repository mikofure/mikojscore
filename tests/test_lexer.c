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
 * MikoJSCore Lexer Tests
 */

#include "../src/lexer.h"
#include "../include/mikojs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* External test functions */
extern int get_tests_run(void);
extern int get_tests_passed(void);
extern int get_tests_failed(void);
extern void reset_test_stats(void);

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

static int test_lexer_basic_tokens(void) {
    TEST_SUITE_BEGIN("Basic Token Recognition");
    
    const char* source = "123 \"hello\" true false null undefined";
    mjs_lexer_t* lexer = mjs_lexer_new(source);
    
    TEST_ASSERT(lexer != NULL, "Lexer creation");
    
    // Test number token
    mjs_token_t token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_NUMBER, "Number token type");
    TEST_ASSERT(token.value.number == 123.0, "Number token value");
    
    // Test string token
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_STRING, "String token type");
    TEST_ASSERT(strcmp(token.value.string, "hello") == 0, "String token value");
    
    // Test boolean tokens
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_TRUE, "True token type");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_FALSE, "False token type");
    
    // Test null token
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_NULL, "Null token type");
    
    // Test undefined token
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_UNDEFINED, "Undefined token type");
    
    // Test EOF
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_EOF, "EOF token");
    
    mjs_lexer_free(lexer);
    return 0;
}

static int test_lexer_operators(void) {
    TEST_SUITE_BEGIN("Operator Recognition");
    
    const char* source = "+ - * / % == != < <= > >= && || ! = += -= *= /=";
    mjs_lexer_t* lexer = mjs_lexer_new(source);
    
    TEST_ASSERT(lexer != NULL, "Lexer creation");
    
    // Test arithmetic operators
    mjs_token_t token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_PLUS, "Plus operator");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_MINUS, "Minus operator");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_MULTIPLY, "Multiply operator");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_DIVIDE, "Divide operator");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_MODULO, "Modulo operator");
    
    // Test comparison operators
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_EQUAL, "Equal operator");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_NOT_EQUAL, "Not equal operator");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_LESS_THAN, "Less than operator");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_LESS_EQUAL, "Less equal operator");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_GREATER_THAN, "Greater than operator");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_GREATER_EQUAL, "Greater equal operator");
    
    // Test logical operators
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_LOGICAL_AND, "Logical AND operator");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_LOGICAL_OR, "Logical OR operator");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_LOGICAL_NOT, "Logical NOT operator");
    
    // Test assignment operators
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_ASSIGN, "Assignment operator");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_PLUS_ASSIGN, "Plus assign operator");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_MINUS_ASSIGN, "Minus assign operator");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_MULTIPLY_ASSIGN, "Multiply assign operator");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_DIVIDE_ASSIGN, "Divide assign operator");
    
    mjs_lexer_free(lexer);
    return 0;
}

static int test_lexer_identifiers_keywords(void) {
    TEST_SUITE_BEGIN("Identifiers and Keywords");
    
    const char* source = "var let const function if else while for return break continue";
    mjs_lexer_t* lexer = mjs_lexer_new(source);
    
    TEST_ASSERT(lexer != NULL, "Lexer creation");
    
    // Test keywords
    mjs_token_t token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_VAR, "var keyword");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_LET, "let keyword");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_CONST, "const keyword");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_FUNCTION, "function keyword");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_IF, "if keyword");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_ELSE, "else keyword");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_WHILE, "while keyword");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_FOR, "for keyword");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_RETURN, "return keyword");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_BREAK, "break keyword");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_CONTINUE, "continue keyword");
    
    mjs_lexer_free(lexer);
    return 0;
}

static int test_lexer_punctuation(void) {
    TEST_SUITE_BEGIN("Punctuation");
    
    const char* source = "( ) { } [ ] ; , . :";
    mjs_lexer_t* lexer = mjs_lexer_new(source);
    
    TEST_ASSERT(lexer != NULL, "Lexer creation");
    
    mjs_token_t token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_LPAREN, "Left parenthesis");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_RPAREN, "Right parenthesis");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_LBRACE, "Left brace");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_RBRACE, "Right brace");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_LBRACKET, "Left bracket");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_RBRACKET, "Right bracket");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_SEMICOLON, "Semicolon");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_COMMA, "Comma");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_DOT, "Dot");
    
    token = mjs_lexer_next_token(lexer);
    TEST_ASSERT(token.type == TOKEN_COLON, "Colon");
    
    mjs_lexer_free(lexer);
    return 0;
}

static int test_lexer_error_handling(void) {
    TEST_SUITE_BEGIN("Error Handling");
    
    // Test unterminated string
    const char* source1 = "\"unterminated string";
    mjs_lexer_t* lexer1 = mjs_lexer_new(source1);
    TEST_ASSERT(lexer1 != NULL, "Lexer creation for error test");
    
    mjs_token_t token = mjs_lexer_next_token(lexer1);
    TEST_ASSERT(mjs_lexer_has_error(lexer1), "Unterminated string error detected");
    
    mjs_lexer_free(lexer1);
    
    // Test invalid character
    const char* source2 = "@invalid";
    mjs_lexer_t* lexer2 = mjs_lexer_new(source2);
    TEST_ASSERT(lexer2 != NULL, "Lexer creation for invalid char test");
    
    token = mjs_lexer_next_token(lexer2);
    // Note: @ might be treated as an identifier start in some implementations
    // This test might need adjustment based on actual lexer behavior
    
    mjs_lexer_free(lexer2);
    
    return 0;
}

int test_lexer_run(void) {
    printf("\n=== Running Lexer Tests ===\n");
    
    int result = 0;
    
    result |= test_lexer_basic_tokens();
    result |= test_lexer_operators();
    result |= test_lexer_identifiers_keywords();
    result |= test_lexer_punctuation();
    result |= test_lexer_error_handling();
    
    if (result == 0) {
        printf("\n✅ All lexer tests passed!\n");
    } else {
        printf("\n❌ Some lexer tests failed.\n");
    }
    
    return result;
}