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
 * MikoJSCore Lexer
 * JavaScript tokenizer for ES2020+ syntax
 */

#ifndef MIKOJS_LEXER_H
#define MIKOJS_LEXER_H

#include "mikojs_internal.h"

/* Token types */
typedef enum {
    /* Literals */
    TOKEN_UNDEFINED,
    TOKEN_NULL,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_TEMPLATE_LITERAL,
    TOKEN_BIGINT,
    
    /* Identifiers and keywords */
    TOKEN_IDENTIFIER,
    TOKEN_KEYWORD_ASYNC,
    TOKEN_KEYWORD_AWAIT,
    TOKEN_KEYWORD_BREAK,
    TOKEN_KEYWORD_CASE,
    TOKEN_KEYWORD_CATCH,
    TOKEN_KEYWORD_CLASS,
    TOKEN_KEYWORD_CONST,
    TOKEN_KEYWORD_CONTINUE,
    TOKEN_KEYWORD_DEBUGGER,
    TOKEN_KEYWORD_DEFAULT,
    TOKEN_KEYWORD_DELETE,
    TOKEN_KEYWORD_DO,
    TOKEN_KEYWORD_ELSE,
    TOKEN_KEYWORD_EXPORT,
    TOKEN_KEYWORD_EXTENDS,
    TOKEN_KEYWORD_FINALLY,
    TOKEN_KEYWORD_FOR,
    TOKEN_KEYWORD_FUNCTION,
    TOKEN_KEYWORD_IF,
    TOKEN_KEYWORD_IMPORT,
    TOKEN_KEYWORD_IN,
    TOKEN_KEYWORD_INSTANCEOF,
    TOKEN_KEYWORD_LET,
    TOKEN_KEYWORD_NEW,
    TOKEN_KEYWORD_OF,
    TOKEN_KEYWORD_RETURN,
    TOKEN_KEYWORD_STATIC,
    TOKEN_KEYWORD_SUPER,
    TOKEN_KEYWORD_SWITCH,
    TOKEN_KEYWORD_THIS,
    TOKEN_KEYWORD_THROW,
    TOKEN_KEYWORD_TRY,
    TOKEN_KEYWORD_TYPEOF,
    TOKEN_KEYWORD_VAR,
    TOKEN_KEYWORD_VOID,
    TOKEN_KEYWORD_WHILE,
    TOKEN_KEYWORD_WITH,
    TOKEN_KEYWORD_YIELD,
    
    /* Operators */
    TOKEN_PLUS,              /* + */
    TOKEN_MINUS,             /* - */
    TOKEN_MULTIPLY,          /* * */
    TOKEN_DIVIDE,            /* / */
    TOKEN_MODULO,            /* % */
    TOKEN_EXPONENT,          /* ** */
    TOKEN_ASSIGN,            /* = */
    TOKEN_PLUS_ASSIGN,       /* += */
    TOKEN_MINUS_ASSIGN,      /* -= */
    TOKEN_MULTIPLY_ASSIGN,   /* *= */
    TOKEN_DIVIDE_ASSIGN,     /* /= */
    TOKEN_MODULO_ASSIGN,     /* %= */
    TOKEN_EXPONENT_ASSIGN,   /* **= */
    TOKEN_INCREMENT,         /* ++ */
    TOKEN_DECREMENT,         /* -- */
    
    /* Comparison operators */
    TOKEN_EQUAL,             /* == */
    TOKEN_NOT_EQUAL,         /* != */
    TOKEN_STRICT_EQUAL,      /* === */
    TOKEN_STRICT_NOT_EQUAL,  /* !== */
    TOKEN_LESS_THAN,         /* < */
    TOKEN_LESS_EQUAL,        /* <= */
    TOKEN_GREATER_THAN,      /* > */
    TOKEN_GREATER_EQUAL,     /* >= */
    
    /* Logical operators */
    TOKEN_LOGICAL_AND,       /* && */
    TOKEN_LOGICAL_OR,        /* || */
    TOKEN_LOGICAL_NOT,       /* ! */
    TOKEN_NULLISH_COALESCING, /* ?? */
    
    /* Bitwise operators */
    TOKEN_BITWISE_AND,       /* & */
    TOKEN_BITWISE_OR,        /* | */
    TOKEN_BITWISE_XOR,       /* ^ */
    TOKEN_BITWISE_NOT,       /* ~ */
    TOKEN_LEFT_SHIFT,        /* << */
    TOKEN_RIGHT_SHIFT,       /* >> */
    TOKEN_UNSIGNED_RIGHT_SHIFT, /* >>> */
    TOKEN_BITWISE_AND_ASSIGN,   /* &= */
    TOKEN_BITWISE_OR_ASSIGN,    /* |= */
    TOKEN_BITWISE_XOR_ASSIGN,   /* ^= */
    TOKEN_LEFT_SHIFT_ASSIGN,    /* <<= */
    TOKEN_RIGHT_SHIFT_ASSIGN,   /* >>= */
    TOKEN_UNSIGNED_RIGHT_SHIFT_ASSIGN, /* >>>= */
    
    /* Punctuation */
    TOKEN_SEMICOLON,         /* ; */
    TOKEN_COMMA,             /* , */
    TOKEN_DOT,               /* . */
    TOKEN_OPTIONAL_CHAINING, /* ?. */
    TOKEN_QUESTION,          /* ? */
    TOKEN_COLON,             /* : */
    TOKEN_ARROW,             /* => */
    
    /* Brackets */
    TOKEN_LEFT_PAREN,        /* ( */
    TOKEN_RIGHT_PAREN,       /* ) */
    TOKEN_LEFT_BRACKET,      /* [ */
    TOKEN_RIGHT_BRACKET,     /* ] */
    TOKEN_LEFT_BRACE,        /* { */
    TOKEN_RIGHT_BRACE,       /* } */
    
    /* Special */
    TOKEN_NEWLINE,
    TOKEN_EOF,
    TOKEN_ERROR
} mjs_token_type_t;

/* Token structure */
typedef struct {
    mjs_token_type_t type;
    const char* start;
    size_t length;
    size_t line;
    size_t column;
    union {
        double number;
        char* string;
        bool boolean;
    } value;
} mjs_token_t;

/* Lexer structure */
struct mjs_lexer {
    const char* source;
    const char* current;
    const char* start;
    size_t line;
    size_t column;
    size_t length;
    mjs_token_t current_token;
    bool has_error;
    char* error_message;
};

/* Lexer functions */
mjs_lexer_t* mjs_lexer_new(const char* source, size_t length);
void mjs_lexer_free(mjs_lexer_t* lexer);

mjs_token_t mjs_lexer_next_token(mjs_lexer_t* lexer);
mjs_token_t mjs_lexer_peek_token(mjs_lexer_t* lexer);
void mjs_lexer_consume_token(mjs_lexer_t* lexer);

bool mjs_lexer_has_error(mjs_lexer_t* lexer);
const char* mjs_lexer_get_error(mjs_lexer_t* lexer);

/* Token utilities */
const char* mjs_token_type_to_string(mjs_token_type_t type);
bool mjs_token_is_keyword(mjs_token_type_t type);
bool mjs_token_is_operator(mjs_token_type_t type);
bool mjs_token_is_assignment(mjs_token_type_t type);

/* Keyword lookup */
mjs_token_type_t mjs_lookup_keyword(const char* identifier, size_t length);

#endif /* MIKOJS_LEXER_H */