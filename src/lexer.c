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
 * MikoJSCore Lexer Implementation
 * JavaScript tokenizer for ES2020+ syntax
 */

#include "lexer.h"
#include <ctype.h>
#include <stdarg.h>

/* Keyword lookup table */
static const struct {
    const char* keyword;
    mjs_token_type_t type;
} keywords[] = {
    {"async", TOKEN_KEYWORD_ASYNC},
    {"await", TOKEN_KEYWORD_AWAIT},
    {"break", TOKEN_KEYWORD_BREAK},
    {"case", TOKEN_KEYWORD_CASE},
    {"catch", TOKEN_KEYWORD_CATCH},
    {"class", TOKEN_KEYWORD_CLASS},
    {"const", TOKEN_KEYWORD_CONST},
    {"continue", TOKEN_KEYWORD_CONTINUE},
    {"debugger", TOKEN_KEYWORD_DEBUGGER},
    {"default", TOKEN_KEYWORD_DEFAULT},
    {"delete", TOKEN_KEYWORD_DELETE},
    {"do", TOKEN_KEYWORD_DO},
    {"else", TOKEN_KEYWORD_ELSE},
    {"export", TOKEN_KEYWORD_EXPORT},
    {"extends", TOKEN_KEYWORD_EXTENDS},
    {"false", TOKEN_FALSE},
    {"finally", TOKEN_KEYWORD_FINALLY},
    {"for", TOKEN_KEYWORD_FOR},
    {"function", TOKEN_KEYWORD_FUNCTION},
    {"if", TOKEN_KEYWORD_IF},
    {"import", TOKEN_KEYWORD_IMPORT},
    {"in", TOKEN_KEYWORD_IN},
    {"instanceof", TOKEN_KEYWORD_INSTANCEOF},
    {"let", TOKEN_KEYWORD_LET},
    {"new", TOKEN_KEYWORD_NEW},
    {"null", TOKEN_NULL},
    {"of", TOKEN_KEYWORD_OF},
    {"return", TOKEN_KEYWORD_RETURN},
    {"static", TOKEN_KEYWORD_STATIC},
    {"super", TOKEN_KEYWORD_SUPER},
    {"switch", TOKEN_KEYWORD_SWITCH},
    {"this", TOKEN_KEYWORD_THIS},
    {"throw", TOKEN_KEYWORD_THROW},
    {"true", TOKEN_TRUE},
    {"try", TOKEN_KEYWORD_TRY},
    {"typeof", TOKEN_KEYWORD_TYPEOF},
    {"undefined", TOKEN_UNDEFINED},
    {"var", TOKEN_KEYWORD_VAR},
    {"void", TOKEN_KEYWORD_VOID},
    {"while", TOKEN_KEYWORD_WHILE},
    {"with", TOKEN_KEYWORD_WITH},
    {"yield", TOKEN_KEYWORD_YIELD}
};

#define KEYWORD_COUNT (sizeof(keywords) / sizeof(keywords[0]))

/* Helper functions */
static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '$';
}

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool is_hex_digit(char c) {
    return is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static bool is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\r';
}

static bool is_newline(char c) {
    return c == '\n';
}

static char peek_char(mjs_lexer_t* lexer, size_t offset) {
    if (lexer->current + offset >= lexer->source + lexer->length) {
        return '\0';
    }
    return lexer->current[offset];
}

static char advance_char(mjs_lexer_t* lexer) {
    if (lexer->current >= lexer->source + lexer->length) {
        return '\0';
    }
    
    char c = *lexer->current++;
    if (is_newline(c)) {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    return c;
}

static void lexer_error(mjs_lexer_t* lexer, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    
    if (lexer->error_message) {
        MJS_FREE(lexer->error_message);
    }
    
    lexer->error_message = MJS_MALLOC(256);
    vsnprintf(lexer->error_message, 256, fmt, args);
    lexer->has_error = true;
    
    va_end(args);
}

static void skip_whitespace(mjs_lexer_t* lexer) {
    while (is_whitespace(peek_char(lexer, 0))) {
        advance_char(lexer);
    }
}

static void skip_line_comment(mjs_lexer_t* lexer) {
    // Skip //
    advance_char(lexer);
    advance_char(lexer);
    
    while (peek_char(lexer, 0) != '\n' && peek_char(lexer, 0) != '\0') {
        advance_char(lexer);
    }
}

static void skip_block_comment(mjs_lexer_t* lexer) {
    // Skip /*
    advance_char(lexer);
    advance_char(lexer);
    
    while (peek_char(lexer, 0) != '\0') {
        if (peek_char(lexer, 0) == '*' && peek_char(lexer, 1) == '/') {
            advance_char(lexer); // *
            advance_char(lexer); // /
            break;
        }
        advance_char(lexer);
    }
}

static mjs_token_t make_token(mjs_lexer_t* lexer, mjs_token_type_t type) {
    mjs_token_t token;
    token.type = type;
    token.start = lexer->start;
    token.length = lexer->current - lexer->start;
    token.line = lexer->line;
    token.column = lexer->column - token.length;
    return token;
}

static mjs_token_t make_error_token(mjs_lexer_t* lexer, const char* message) {
    lexer_error(lexer, "%s", message);
    mjs_token_t token;
    token.type = TOKEN_ERROR;
    token.start = lexer->start;
    token.length = lexer->current - lexer->start;
    token.line = lexer->line;
    token.column = lexer->column;
    return token;
}

static mjs_token_t scan_string(mjs_lexer_t* lexer, char quote) {
    while (peek_char(lexer, 0) != quote && peek_char(lexer, 0) != '\0') {
        if (peek_char(lexer, 0) == '\n') {
            return make_error_token(lexer, "Unterminated string");
        }
        if (peek_char(lexer, 0) == '\\') {
            advance_char(lexer); // Skip escape character
            if (peek_char(lexer, 0) != '\0') {
                advance_char(lexer); // Skip escaped character
            }
        } else {
            advance_char(lexer);
        }
    }
    
    if (peek_char(lexer, 0) == '\0') {
        return make_error_token(lexer, "Unterminated string");
    }
    
    advance_char(lexer); // Closing quote
    return make_token(lexer, TOKEN_STRING);
}

static mjs_token_t scan_number(mjs_lexer_t* lexer) {
    // Handle hex numbers
    if (peek_char(lexer, 0) == '0' && (peek_char(lexer, 1) == 'x' || peek_char(lexer, 1) == 'X')) {
        advance_char(lexer); // 0
        advance_char(lexer); // x
        
        if (!is_hex_digit(peek_char(lexer, 0))) {
            return make_error_token(lexer, "Invalid hex number");
        }
        
        while (is_hex_digit(peek_char(lexer, 0))) {
            advance_char(lexer);
        }
        return make_token(lexer, TOKEN_NUMBER);
    }
    
    // Handle binary numbers
    if (peek_char(lexer, 0) == '0' && (peek_char(lexer, 1) == 'b' || peek_char(lexer, 1) == 'B')) {
        advance_char(lexer); // 0
        advance_char(lexer); // b
        
        if (peek_char(lexer, 0) != '0' && peek_char(lexer, 0) != '1') {
            return make_error_token(lexer, "Invalid binary number");
        }
        
        while (peek_char(lexer, 0) == '0' || peek_char(lexer, 0) == '1') {
            advance_char(lexer);
        }
        return make_token(lexer, TOKEN_NUMBER);
    }
    
    // Handle octal numbers
    if (peek_char(lexer, 0) == '0' && (peek_char(lexer, 1) == 'o' || peek_char(lexer, 1) == 'O')) {
        advance_char(lexer); // 0
        advance_char(lexer); // o
        
        if (peek_char(lexer, 0) < '0' || peek_char(lexer, 0) > '7') {
            return make_error_token(lexer, "Invalid octal number");
        }
        
        while (peek_char(lexer, 0) >= '0' && peek_char(lexer, 0) <= '7') {
            advance_char(lexer);
        }
        return make_token(lexer, TOKEN_NUMBER);
    }
    
    // Handle decimal numbers
    while (is_digit(peek_char(lexer, 0))) {
        advance_char(lexer);
    }
    
    // Handle decimal point
    if (peek_char(lexer, 0) == '.' && is_digit(peek_char(lexer, 1))) {
        advance_char(lexer); // .
        while (is_digit(peek_char(lexer, 0))) {
            advance_char(lexer);
        }
    }
    
    // Handle exponent
    if (peek_char(lexer, 0) == 'e' || peek_char(lexer, 0) == 'E') {
        advance_char(lexer);
        if (peek_char(lexer, 0) == '+' || peek_char(lexer, 0) == '-') {
            advance_char(lexer);
        }
        if (!is_digit(peek_char(lexer, 0))) {
            return make_error_token(lexer, "Invalid number exponent");
        }
        while (is_digit(peek_char(lexer, 0))) {
            advance_char(lexer);
        }
    }
    
    // Check for BigInt suffix
    if (peek_char(lexer, 0) == 'n') {
        advance_char(lexer);
        return make_token(lexer, TOKEN_BIGINT);
    }
    
    return make_token(lexer, TOKEN_NUMBER);
}

static mjs_token_t scan_identifier(mjs_lexer_t* lexer) {
    while (is_alpha(peek_char(lexer, 0)) || is_digit(peek_char(lexer, 0))) {
        advance_char(lexer);
    }
    
    // Check if it's a keyword
    size_t length = lexer->current - lexer->start;
    for (size_t i = 0; i < KEYWORD_COUNT; i++) {
        if (strlen(keywords[i].keyword) == length &&
            memcmp(lexer->start, keywords[i].keyword, length) == 0) {
            return make_token(lexer, keywords[i].type);
        }
    }
    
    return make_token(lexer, TOKEN_IDENTIFIER);
}

/* Public functions */
mjs_lexer_t* mjs_lexer_new(const char* source, size_t length) {
    mjs_lexer_t* lexer = MJS_MALLOC(sizeof(mjs_lexer_t));
    if (!lexer) return NULL;
    
    lexer->source = source;
    lexer->current = source;
    lexer->start = source;
    lexer->line = 1;
    lexer->column = 1;
    lexer->length = length;
    lexer->has_error = false;
    lexer->error_message = NULL;
    
    return lexer;
}

void mjs_lexer_free(mjs_lexer_t* lexer) {
    if (!lexer) return;
    
    if (lexer->error_message) {
        MJS_FREE(lexer->error_message);
    }
    MJS_FREE(lexer);
}

mjs_token_t mjs_lexer_next_token(mjs_lexer_t* lexer) {
    skip_whitespace(lexer);
    
    lexer->start = lexer->current;
    
    if (lexer->current >= lexer->source + lexer->length) {
        return make_token(lexer, TOKEN_EOF);
    }
    
    char c = advance_char(lexer);
    
    // Handle newlines
    if (is_newline(c)) {
        return make_token(lexer, TOKEN_NEWLINE);
    }
    
    // Handle identifiers and keywords
    if (is_alpha(c)) {
        lexer->current--; // Back up
        return scan_identifier(lexer);
    }
    
    // Handle numbers
    if (is_digit(c)) {
        lexer->current--; // Back up
        return scan_number(lexer);
    }
    
    // Handle single-character tokens and operators
    switch (c) {
        case '(': return make_token(lexer, TOKEN_LEFT_PAREN);
        case ')': return make_token(lexer, TOKEN_RIGHT_PAREN);
        case '[': return make_token(lexer, TOKEN_LEFT_BRACKET);
        case ']': return make_token(lexer, TOKEN_RIGHT_BRACKET);
        case '{': return make_token(lexer, TOKEN_LEFT_BRACE);
        case '}': return make_token(lexer, TOKEN_RIGHT_BRACE);
        case ';': return make_token(lexer, TOKEN_SEMICOLON);
        case ',': return make_token(lexer, TOKEN_COMMA);
        case '~': return make_token(lexer, TOKEN_BITWISE_NOT);
        
        case '+': {
            if (peek_char(lexer, 0) == '+') {
                advance_char(lexer);
                return make_token(lexer, TOKEN_INCREMENT);
            } else if (peek_char(lexer, 0) == '=') {
                advance_char(lexer);
                return make_token(lexer, TOKEN_PLUS_ASSIGN);
            }
            return make_token(lexer, TOKEN_PLUS);
        }
        
        case '-': {
            if (peek_char(lexer, 0) == '-') {
                advance_char(lexer);
                return make_token(lexer, TOKEN_DECREMENT);
            } else if (peek_char(lexer, 0) == '=') {
                advance_char(lexer);
                return make_token(lexer, TOKEN_MINUS_ASSIGN);
            }
            return make_token(lexer, TOKEN_MINUS);
        }
        
        case '*': {
            if (peek_char(lexer, 0) == '*') {
                advance_char(lexer);
                if (peek_char(lexer, 0) == '=') {
                    advance_char(lexer);
                    return make_token(lexer, TOKEN_EXPONENT_ASSIGN);
                }
                return make_token(lexer, TOKEN_EXPONENT);
            } else if (peek_char(lexer, 0) == '=') {
                advance_char(lexer);
                return make_token(lexer, TOKEN_MULTIPLY_ASSIGN);
            }
            return make_token(lexer, TOKEN_MULTIPLY);
        }
        
        case '/': {
            if (peek_char(lexer, 0) == '/') {
                skip_line_comment(lexer);
                return mjs_lexer_next_token(lexer); // Recursive call to get next token
            } else if (peek_char(lexer, 0) == '*') {
                skip_block_comment(lexer);
                return mjs_lexer_next_token(lexer); // Recursive call to get next token
            } else if (peek_char(lexer, 0) == '=') {
                advance_char(lexer);
                return make_token(lexer, TOKEN_DIVIDE_ASSIGN);
            }
            return make_token(lexer, TOKEN_DIVIDE);
        }
        
        case '%': {
            if (peek_char(lexer, 0) == '=') {
                advance_char(lexer);
                return make_token(lexer, TOKEN_MODULO_ASSIGN);
            }
            return make_token(lexer, TOKEN_MODULO);
        }
        
        case '=': {
            if (peek_char(lexer, 0) == '=') {
                advance_char(lexer);
                if (peek_char(lexer, 0) == '=') {
                    advance_char(lexer);
                    return make_token(lexer, TOKEN_STRICT_EQUAL);
                }
                return make_token(lexer, TOKEN_EQUAL);
            } else if (peek_char(lexer, 0) == '>') {
                advance_char(lexer);
                return make_token(lexer, TOKEN_ARROW);
            }
            return make_token(lexer, TOKEN_ASSIGN);
        }
        
        case '!': {
            if (peek_char(lexer, 0) == '=') {
                advance_char(lexer);
                if (peek_char(lexer, 0) == '=') {
                    advance_char(lexer);
                    return make_token(lexer, TOKEN_STRICT_NOT_EQUAL);
                }
                return make_token(lexer, TOKEN_NOT_EQUAL);
            }
            return make_token(lexer, TOKEN_LOGICAL_NOT);
        }
        
        case '<': {
            if (peek_char(lexer, 0) == '<') {
                advance_char(lexer);
                if (peek_char(lexer, 0) == '=') {
                    advance_char(lexer);
                    return make_token(lexer, TOKEN_LEFT_SHIFT_ASSIGN);
                }
                return make_token(lexer, TOKEN_LEFT_SHIFT);
            } else if (peek_char(lexer, 0) == '=') {
                advance_char(lexer);
                return make_token(lexer, TOKEN_LESS_EQUAL);
            }
            return make_token(lexer, TOKEN_LESS_THAN);
        }
        
        case '>': {
            if (peek_char(lexer, 0) == '>') {
                advance_char(lexer);
                if (peek_char(lexer, 0) == '>') {
                    advance_char(lexer);
                    if (peek_char(lexer, 0) == '=') {
                        advance_char(lexer);
                        return make_token(lexer, TOKEN_UNSIGNED_RIGHT_SHIFT_ASSIGN);
                    }
                    return make_token(lexer, TOKEN_UNSIGNED_RIGHT_SHIFT);
                } else if (peek_char(lexer, 0) == '=') {
                    advance_char(lexer);
                    return make_token(lexer, TOKEN_RIGHT_SHIFT_ASSIGN);
                }
                return make_token(lexer, TOKEN_RIGHT_SHIFT);
            } else if (peek_char(lexer, 0) == '=') {
                advance_char(lexer);
                return make_token(lexer, TOKEN_GREATER_EQUAL);
            }
            return make_token(lexer, TOKEN_GREATER_THAN);
        }
        
        case '&': {
            if (peek_char(lexer, 0) == '&') {
                advance_char(lexer);
                return make_token(lexer, TOKEN_LOGICAL_AND);
            } else if (peek_char(lexer, 0) == '=') {
                advance_char(lexer);
                return make_token(lexer, TOKEN_BITWISE_AND_ASSIGN);
            }
            return make_token(lexer, TOKEN_BITWISE_AND);
        }
        
        case '|': {
            if (peek_char(lexer, 0) == '|') {
                advance_char(lexer);
                return make_token(lexer, TOKEN_LOGICAL_OR);
            } else if (peek_char(lexer, 0) == '=') {
                advance_char(lexer);
                return make_token(lexer, TOKEN_BITWISE_OR_ASSIGN);
            }
            return make_token(lexer, TOKEN_BITWISE_OR);
        }
        
        case '^': {
            if (peek_char(lexer, 0) == '=') {
                advance_char(lexer);
                return make_token(lexer, TOKEN_BITWISE_XOR_ASSIGN);
            }
            return make_token(lexer, TOKEN_BITWISE_XOR);
        }
        
        case '?': {
            if (peek_char(lexer, 0) == '?') {
                advance_char(lexer);
                return make_token(lexer, TOKEN_NULLISH_COALESCING);
            } else if (peek_char(lexer, 0) == '.') {
                advance_char(lexer);
                return make_token(lexer, TOKEN_OPTIONAL_CHAINING);
            }
            return make_token(lexer, TOKEN_QUESTION);
        }
        
        case ':': return make_token(lexer, TOKEN_COLON);
        
        case '.': {
            if (is_digit(peek_char(lexer, 0))) {
                lexer->current--; // Back up
                return scan_number(lexer);
            }
            return make_token(lexer, TOKEN_DOT);
        }
        
        case '"':
        case '\'':
            return scan_string(lexer, c);
        
        default:
            return make_error_token(lexer, "Unexpected character");
    }
}

mjs_token_t mjs_lexer_peek_token(mjs_lexer_t* lexer) {
    // Save current state
    const char* saved_current = lexer->current;
    const char* saved_start = lexer->start;
    size_t saved_line = lexer->line;
    size_t saved_column = lexer->column;
    
    // Get next token
    mjs_token_t token = mjs_lexer_next_token(lexer);
    
    // Restore state
    lexer->current = saved_current;
    lexer->start = saved_start;
    lexer->line = saved_line;
    lexer->column = saved_column;
    
    return token;
}

void mjs_lexer_consume_token(mjs_lexer_t* lexer) {
    lexer->current_token = mjs_lexer_next_token(lexer);
}

bool mjs_lexer_has_error(mjs_lexer_t* lexer) {
    return lexer->has_error;
}

const char* mjs_lexer_get_error(mjs_lexer_t* lexer) {
    return lexer->error_message;
}

/* Token utilities */
const char* mjs_token_type_to_string(mjs_token_type_t type) {
    switch (type) {
        case TOKEN_UNDEFINED: return "undefined";
        case TOKEN_NULL: return "null";
        case TOKEN_TRUE: return "true";
        case TOKEN_FALSE: return "false";
        case TOKEN_NUMBER: return "number";
        case TOKEN_STRING: return "string";
        case TOKEN_BIGINT: return "bigint";
        case TOKEN_IDENTIFIER: return "identifier";
        case TOKEN_PLUS: return "+";
        case TOKEN_MINUS: return "-";
        case TOKEN_MULTIPLY: return "*";
        case TOKEN_DIVIDE: return "/";
        case TOKEN_ASSIGN: return "=";
        case TOKEN_EQUAL: return "==";
        case TOKEN_STRICT_EQUAL: return "===";
        case TOKEN_LEFT_PAREN: return "(";
        case TOKEN_RIGHT_PAREN: return ")";
        case TOKEN_LEFT_BRACE: return "{";
        case TOKEN_RIGHT_BRACE: return "}";
        case TOKEN_SEMICOLON: return ";";
        case TOKEN_EOF: return "EOF";
        case TOKEN_ERROR: return "ERROR";
        default: return "unknown";
    }
}

bool mjs_token_is_keyword(mjs_token_type_t type) {
    return type >= TOKEN_KEYWORD_ASYNC && type <= TOKEN_KEYWORD_YIELD;
}

bool mjs_token_is_operator(mjs_token_type_t type) {
    return type >= TOKEN_PLUS && type <= TOKEN_UNSIGNED_RIGHT_SHIFT_ASSIGN;
}

bool mjs_token_is_assignment(mjs_token_type_t type) {
    return type == TOKEN_ASSIGN ||
           (type >= TOKEN_PLUS_ASSIGN && type <= TOKEN_UNSIGNED_RIGHT_SHIFT_ASSIGN);
}

mjs_token_type_t mjs_lookup_keyword(const char* identifier, size_t length) {
    for (size_t i = 0; i < KEYWORD_COUNT; i++) {
        if (strlen(keywords[i].keyword) == length &&
            memcmp(identifier, keywords[i].keyword, length) == 0) {
            return keywords[i].type;
        }
    }
    return TOKEN_IDENTIFIER;
}