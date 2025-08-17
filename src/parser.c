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
 * MikoJSCore Parser Implementation
 * Recursive descent parser for JavaScript syntax
 */

#include "parser.h"
#include "lexer.h"
#include "mikojs_internal.h"
#include <string.h>

/* Forward declarations for recursive parsing functions */
static mjs_ast_node_t* parse_expression(mjs_parser_t* parser);
static mjs_ast_node_t* parse_assignment_expression(mjs_parser_t* parser);
static mjs_ast_node_t* parse_conditional_expression(mjs_parser_t* parser);
static mjs_ast_node_t* parse_logical_or_expression(mjs_parser_t* parser);
static mjs_ast_node_t* parse_logical_and_expression(mjs_parser_t* parser);
static mjs_ast_node_t* parse_bitwise_or_expression(mjs_parser_t* parser);
static mjs_ast_node_t* parse_bitwise_xor_expression(mjs_parser_t* parser);
static mjs_ast_node_t* parse_bitwise_and_expression(mjs_parser_t* parser);
static mjs_ast_node_t* parse_equality_expression(mjs_parser_t* parser);
static mjs_ast_node_t* parse_relational_expression(mjs_parser_t* parser);
static mjs_ast_node_t* parse_shift_expression(mjs_parser_t* parser);
static mjs_ast_node_t* parse_additive_expression(mjs_parser_t* parser);
static mjs_ast_node_t* parse_multiplicative_expression(mjs_parser_t* parser);
static mjs_ast_node_t* parse_unary_expression(mjs_parser_t* parser);
static mjs_ast_node_t* parse_postfix_expression(mjs_parser_t* parser);
static mjs_ast_node_t* parse_primary_expression(mjs_parser_t* parser);
static mjs_ast_node_t* parse_statement(mjs_parser_t* parser);
static mjs_ast_node_t* parse_block_statement(mjs_parser_t* parser);
static mjs_ast_node_t* parse_variable_declaration(mjs_parser_t* parser);
static mjs_ast_node_t* parse_function_declaration(mjs_parser_t* parser);

/* Parser creation and destruction */
mjs_parser_t* mjs_parser_new(mjs_context_t* ctx, const char* source, size_t length) {
    if (!ctx || !source) return NULL;
    
    mjs_parser_t* parser = MJS_MALLOC(sizeof(mjs_parser_t));
    if (!parser) return NULL;
    
    parser->lexer = mjs_lexer_new(source, strlen(source));
    if (!parser->lexer) {
        MJS_FREE(parser);
        return NULL;
    }
    
    parser->current_token = mjs_lexer_next_token(parser->lexer);
    parser->has_error = false;
    parser->error_message[0] = '\0';
    parser->context = ctx;
    
    return parser;
}

void mjs_parser_free(mjs_parser_t* parser) {
    if (!parser) return;
    
    if (parser->lexer) {
        mjs_lexer_free(parser->lexer);
    }
    
    MJS_FREE(parser);
}

/* Error handling */
static void parser_error(mjs_parser_t* parser, const char* message) {
    if (!parser || parser->has_error) return;
    
    parser->has_error = true;
    snprintf(parser->error_message, sizeof(parser->error_message), 
             "Parse error at line %zu, column %zu: %s", 
             parser->current_token.line, parser->current_token.column, message);
}

bool mjs_parser_has_error(mjs_parser_t* parser) {
    return parser ? parser->has_error : true;
}

const char* mjs_parser_get_error(mjs_parser_t* parser) {
    return parser ? parser->error_message : "Invalid parser";
}

/* Token consumption helpers */
static bool consume_token(mjs_parser_t* parser, mjs_token_type_t expected) {
    if (parser->current_token.type == expected) {
        parser->current_token = mjs_lexer_next_token(parser->lexer);
        return true;
    }
    return false;
}

static bool expect_token(mjs_parser_t* parser, mjs_token_type_t expected) {
    if (consume_token(parser, expected)) {
        return true;
    }
    
    char error_msg[256];
    snprintf(error_msg, sizeof(error_msg), "Expected %s, got %s",
             mjs_token_type_to_string(expected),
             mjs_token_type_to_string(parser->current_token.type));
    parser_error(parser, error_msg);
    return false;
}

/* AST node creation helpers */
static mjs_ast_node_t* create_ast_node(mjs_ast_type_t type) {
    mjs_ast_node_t* node = MJS_MALLOC(sizeof(mjs_ast_node_t));
    if (!node) return NULL;
    
    memset(node, 0, sizeof(mjs_ast_node_t));
    node->type = type;
    return node;
}

/* Primary expressions */
static mjs_ast_node_t* parse_primary_expression(mjs_parser_t* parser) {
    mjs_ast_node_t* node = NULL;
    
    switch (parser->current_token.type) {
        case TOKEN_NUMBER:
            node = create_ast_node(AST_LITERAL_NUMBER);
            if (node) {
                node->u.literal.value = mjs_value_number(parser->current_token.value.number);
                consume_token(parser, TOKEN_NUMBER);
            }
            break;
            
        case TOKEN_STRING:
            node = create_ast_node(AST_LITERAL_STRING);
            if (node) {
                mjs_string_t* str = mjs_string_new(parser->context, parser->current_token.value.string, strlen(parser->current_token.value.string));
                node->u.literal.value = mjs_value_string(str);
                consume_token(parser, TOKEN_STRING);
            }
            break;
            
        case TOKEN_TRUE:
        case TOKEN_FALSE:
            node = create_ast_node(AST_LITERAL_BOOLEAN);
            if (node) {
                node->u.literal.value = mjs_value_boolean(parser->current_token.type == TOKEN_TRUE);
                consume_token(parser, parser->current_token.type);
            }
            break;
            
        case TOKEN_NULL:
            node = create_ast_node(AST_LITERAL_NULL);
            if (node) {
                node->u.literal.value = mjs_value_null();
                consume_token(parser, TOKEN_NULL);
            }
            break;
            
        case TOKEN_UNDEFINED:
            node = create_ast_node(AST_LITERAL_UNDEFINED);
            if (node) {
                node->u.literal.value = mjs_value_undefined();
                consume_token(parser, TOKEN_UNDEFINED);
            }
            break;
            
        case TOKEN_IDENTIFIER:
            node = create_ast_node(AST_IDENTIFIER);
            if (node) {
                node->u.identifier.name = MJS_STRDUP(parser->current_token.value.string);
                consume_token(parser, TOKEN_IDENTIFIER);
            }
            break;
            
        case TOKEN_LEFT_PAREN:
            consume_token(parser, TOKEN_LEFT_PAREN);
            node = parse_expression(parser);
            expect_token(parser, TOKEN_RIGHT_PAREN);
            break;
            
        case TOKEN_LEFT_BRACKET: {
            // Array literal
            consume_token(parser, TOKEN_LEFT_BRACKET);
            node = create_ast_node(AST_ARRAY_EXPRESSION);
            if (node) {
                node->u.array.elements = NULL;
                 node->u.array.element_count = 0;
                
                // Parse array elements
                if (parser->current_token.type != TOKEN_RIGHT_BRACKET) {
                    // TODO: Implement array element parsing
                    // For now, just skip to closing bracket
                }
                
                expect_token(parser, TOKEN_RIGHT_BRACKET);
            }
            break;
        }
        
        case TOKEN_LEFT_BRACE: {
            // Object literal
            consume_token(parser, TOKEN_LEFT_BRACE);
            node = create_ast_node(AST_OBJECT_EXPRESSION);
            if (node) {
                node->u.object.properties = NULL;
                 node->u.object.property_count = 0;
                
                // Parse object properties
                if (parser->current_token.type != TOKEN_RIGHT_BRACE) {
                    // TODO: Implement object property parsing
                    // For now, just skip to closing brace
                }
                
                expect_token(parser, TOKEN_RIGHT_BRACE);
            }
            break;
        }
        
        default:
            parser_error(parser, "Unexpected token in primary expression");
            break;
    }
    
    return node;
}

/* Postfix expressions (member access, function calls, etc.) */
static mjs_ast_node_t* parse_postfix_expression(mjs_parser_t* parser) {
    mjs_ast_node_t* node = parse_primary_expression(parser);
    if (!node) return NULL;
    
    while (true) {
        switch (parser->current_token.type) {
            case TOKEN_DOT: {
                consume_token(parser, TOKEN_DOT);
                if (parser->current_token.type != TOKEN_IDENTIFIER) {
                    parser_error(parser, "Expected property name after '.'");
                    mjs_ast_node_free(node);
                    return NULL;
                }
                
                mjs_ast_node_t* member_node = create_ast_node(AST_MEMBER_EXPRESSION);
                if (!member_node) {
                    mjs_ast_node_free(node);
                    return NULL;
                }
                
                member_node->u.member.object = node;
            member_node->u.member.property = create_ast_node(AST_IDENTIFIER);
            if (member_node->u.member.property) {
                member_node->u.member.property->u.identifier.name =
                    MJS_STRDUP(parser->current_token.value.string);
            }
            member_node->u.member.computed = false;
                
                consume_token(parser, TOKEN_IDENTIFIER);
                node = member_node;
                break;
            }
            
            case TOKEN_LEFT_BRACKET: {
                consume_token(parser, TOKEN_LEFT_BRACKET);
                
                mjs_ast_node_t* member_node = create_ast_node(AST_MEMBER_EXPRESSION);
                if (!member_node) {
                    mjs_ast_node_free(node);
                    return NULL;
                }
                
                member_node->u.member.object = node;
            member_node->u.member.property = parse_expression(parser);
            member_node->u.member.computed = true;
                
                expect_token(parser, TOKEN_RIGHT_BRACKET);
                node = member_node;
                break;
            }
            
            case TOKEN_LEFT_PAREN: {
                consume_token(parser, TOKEN_LEFT_PAREN);
                
                mjs_ast_node_t* call_node = create_ast_node(AST_CALL_EXPRESSION);
                if (!call_node) {
                    mjs_ast_node_free(node);
                    return NULL;
                }
                
                call_node->u.call.callee = node;
            call_node->u.call.arguments = NULL;
            call_node->u.call.argument_count = 0;
                
                // Parse arguments
                if (parser->current_token.type != TOKEN_RIGHT_PAREN) {
                    // TODO: Implement argument parsing
                    // For now, just skip to closing paren
                }
                
                expect_token(parser, TOKEN_RIGHT_PAREN);
                node = call_node;
                break;
            }
            
            case TOKEN_INCREMENT:
            case TOKEN_DECREMENT: {
                mjs_ast_node_t* update_node = create_ast_node(AST_UPDATE_EXPRESSION);
                if (!update_node) {
                    mjs_ast_free(node);
                    return NULL;
                }
                
                update_node->u.update.argument = node;
            update_node->u.update.operator = parser->current_token.type;
            update_node->u.update.prefix = false;
                
                consume_token(parser, parser->current_token.type);
                node = update_node;
                break;
            }
            
            default:
                return node;
        }
    }
}

/* Unary expressions */
static mjs_ast_node_t* parse_unary_expression(mjs_parser_t* parser) {
    switch (parser->current_token.type) {
        case TOKEN_PLUS:
        case TOKEN_MINUS:
        case TOKEN_LOGICAL_NOT:
        case TOKEN_BITWISE_NOT:
        case TOKEN_INCREMENT:
        case TOKEN_DECREMENT:
        case TOKEN_KEYWORD_TYPEOF:
        case TOKEN_KEYWORD_VOID:
        case TOKEN_KEYWORD_DELETE: {
            mjs_token_type_t op_type = parser->current_token.type;
            consume_token(parser, op_type);
            
            mjs_ast_node_t* node = create_ast_node(AST_UNARY_EXPRESSION);
            if (!node) return NULL;
            
            node->u.unary.argument = parse_unary_expression(parser);
        
        node->u.unary.operator = op_type;
            node->u.unary.prefix = true;
            return node;
        }
        
        default:
            return parse_postfix_expression(parser);
    }
}

/* Binary expression parsing with precedence */
static mjs_ast_node_t* parse_multiplicative_expression(mjs_parser_t* parser) {
    mjs_ast_node_t* left = parse_unary_expression(parser);
    if (!left) return NULL;
    
    while (parser->current_token.type == TOKEN_MULTIPLY ||
           parser->current_token.type == TOKEN_DIVIDE ||
           parser->current_token.type == TOKEN_MODULO) {
        
        mjs_token_type_t op_type = parser->current_token.type;
        consume_token(parser, op_type);
        
        mjs_ast_node_t* right = parse_unary_expression(parser);
        if (!right) {
            mjs_ast_node_free(left);
            return NULL;
        }
        
        mjs_ast_node_t* binary_node = create_ast_node(AST_BINARY_EXPRESSION);
        if (!binary_node) {
            mjs_ast_node_free(left);
        mjs_ast_node_free(right);
            return NULL;
        }
        
        binary_node->u.binary.left = left;
        binary_node->u.binary.right = right;
        binary_node->u.binary.operator = op_type;
        
        left = binary_node;
    }
    
    return left;
}

static mjs_ast_node_t* parse_additive_expression(mjs_parser_t* parser) {
    mjs_ast_node_t* left = parse_multiplicative_expression(parser);
    if (!left) return NULL;
    
    while (parser->current_token.type == TOKEN_PLUS ||
           parser->current_token.type == TOKEN_MINUS) {
        
        mjs_token_type_t op_type = parser->current_token.type;
        consume_token(parser, op_type);
        
        mjs_ast_node_t* right = parse_multiplicative_expression(parser);
        if (!right) {
            mjs_ast_free(left);
            return NULL;
        }
        
        mjs_ast_node_t* binary_node = create_ast_node(AST_BINARY_EXPRESSION);
        if (!binary_node) {
            mjs_ast_free(left);
            mjs_ast_free(right);
            return NULL;
        }
        
        binary_node->u.binary.left = left;
        binary_node->u.binary.right = right;
        binary_node->u.binary.operator = op_type;
        
        left = binary_node;
    }
    
    return left;
}

/* Continue with other precedence levels... */
static mjs_ast_node_t* parse_shift_expression(mjs_parser_t* parser) {
    return parse_additive_expression(parser); // Simplified for now
}

static mjs_ast_node_t* parse_relational_expression(mjs_parser_t* parser) {
    return parse_shift_expression(parser); // Simplified for now
}

static mjs_ast_node_t* parse_equality_expression(mjs_parser_t* parser) {
    return parse_relational_expression(parser); // Simplified for now
}

static mjs_ast_node_t* parse_bitwise_and_expression(mjs_parser_t* parser) {
    return parse_equality_expression(parser); // Simplified for now
}

static mjs_ast_node_t* parse_bitwise_xor_expression(mjs_parser_t* parser) {
    return parse_bitwise_and_expression(parser); // Simplified for now
}

static mjs_ast_node_t* parse_bitwise_or_expression(mjs_parser_t* parser) {
    return parse_bitwise_xor_expression(parser); // Simplified for now
}

static mjs_ast_node_t* parse_logical_and_expression(mjs_parser_t* parser) {
    return parse_bitwise_or_expression(parser); // Simplified for now
}

static mjs_ast_node_t* parse_logical_or_expression(mjs_parser_t* parser) {
    return parse_logical_and_expression(parser); // Simplified for now
}

static mjs_ast_node_t* parse_conditional_expression(mjs_parser_t* parser) {
    return parse_logical_or_expression(parser); // Simplified for now
}

static mjs_ast_node_t* parse_assignment_expression(mjs_parser_t* parser) {
    return parse_conditional_expression(parser); // Simplified for now
}

static mjs_ast_node_t* parse_expression(mjs_parser_t* parser) {
    return parse_assignment_expression(parser);
}

/* Statement parsing */
static mjs_ast_node_t* parse_expression_statement(mjs_parser_t* parser) {
    mjs_ast_node_t* expr = parse_expression(parser);
    if (!expr) return NULL;
    
    expect_token(parser, TOKEN_SEMICOLON);
    
    mjs_ast_node_t* stmt = create_ast_node(AST_EXPRESSION_STATEMENT);
    if (stmt) {
        stmt->u.expr_stmt.expression = expr;
    } else {
        mjs_ast_node_free(expr);
    }
    
    return stmt;
}

static mjs_ast_node_t* parse_block_statement(mjs_parser_t* parser) {
    expect_token(parser, TOKEN_LEFT_BRACE);
    
    mjs_ast_node_t* block = create_ast_node(AST_BLOCK_STATEMENT);
    if (!block) return NULL;
    
    block->u.block.statements = NULL;
    block->u.block.statement_count = 0;
    
    // Parse statements until closing brace
    while (parser->current_token.type != TOKEN_RIGHT_BRACE && 
           parser->current_token.type != TOKEN_EOF) {
        mjs_ast_node_t* stmt = parse_statement(parser);
        if (!stmt) {
            mjs_ast_node_free(block);
            return NULL;
        }
        
        // TODO: Add statement to block's statement list
        // For now, just free the statement
        mjs_ast_node_free(stmt);
    }
    
    expect_token(parser, TOKEN_RIGHT_BRACE);
    return block;
}

static mjs_ast_node_t* parse_statement(mjs_parser_t* parser) {
    switch (parser->current_token.type) {
        case TOKEN_LEFT_BRACE:
            return parse_block_statement(parser);
            
        case TOKEN_KEYWORD_VAR:
        case TOKEN_KEYWORD_LET:
        case TOKEN_KEYWORD_CONST:
            return parse_variable_declaration(parser);
            
        case TOKEN_KEYWORD_FUNCTION:
            return parse_function_declaration(parser);
            
        case TOKEN_KEYWORD_IF:
        case TOKEN_KEYWORD_WHILE:
        case TOKEN_KEYWORD_FOR:
        case TOKEN_KEYWORD_RETURN:
        case TOKEN_KEYWORD_BREAK:
        case TOKEN_KEYWORD_CONTINUE:
            // TODO: Implement control flow statements
            parser_error(parser, "Control flow statements not yet implemented");
            return NULL;
            
        default:
            return parse_expression_statement(parser);
    }
}

static mjs_ast_node_t* parse_variable_declaration(mjs_parser_t* parser) {
    mjs_token_type_t decl_type = parser->current_token.type;
    consume_token(parser, decl_type);
    
    mjs_ast_node_t* decl = create_ast_node(AST_VARIABLE_DECLARATION);
    if (!decl) return NULL;
    
    decl->u.var_decl.kind = (decl_type == TOKEN_KEYWORD_VAR) ? VAR_DECLARATION_VAR : 
                            (decl_type == TOKEN_KEYWORD_LET) ? VAR_DECLARATION_LET : VAR_DECLARATION_CONST;
    decl->u.var_decl.declarations = NULL;
    decl->u.var_decl.declaration_count = 0;
    
    // TODO: Parse variable declarators
    
    expect_token(parser, TOKEN_SEMICOLON);
    return decl;
}

static mjs_ast_node_t* parse_function_declaration(mjs_parser_t* parser) {
    expect_token(parser, TOKEN_KEYWORD_FUNCTION);
    
    mjs_ast_node_t* func = create_ast_node(AST_FUNCTION_DECLARATION);
    if (!func) return NULL;
    
    // Parse function name
    if (parser->current_token.type == TOKEN_IDENTIFIER) {
        func->u.function.name = MJS_STRDUP(parser->current_token.value.string);
        consume_token(parser, TOKEN_IDENTIFIER);
    }
    
    // Parse parameters
    expect_token(parser, TOKEN_LEFT_PAREN);
    func->u.function.params = NULL;
    func->u.function.param_count = 0;
    
    // TODO: Parse parameter list
    
    expect_token(parser, TOKEN_RIGHT_PAREN);
    
    // Parse function body
    func->u.function.body = parse_block_statement(parser);
    
    return func;
}

/* Public parsing functions */
mjs_ast_node_t* mjs_parse_expression(mjs_parser_t* parser) {
    if (!parser || parser->has_error) return NULL;
    return parse_expression(parser);
}

mjs_ast_node_t* mjs_parse_statement(mjs_parser_t* parser) {
    if (!parser || parser->has_error) return NULL;
    return parse_statement(parser);
}

mjs_ast_node_t* mjs_parse_program(mjs_parser_t* parser) {
    if (!parser || parser->has_error) return NULL;
    
    mjs_ast_node_t* program = create_ast_node(AST_PROGRAM);
    if (!program) return NULL;
    
    program->u.program.body = NULL;
    program->u.program.statement_count = 0;
    
    // Parse all statements until EOF
    while (parser->current_token.type != TOKEN_EOF) {
        mjs_ast_node_t* stmt = parse_statement(parser);
        if (!stmt) {
            mjs_ast_node_free(program);
            return NULL;
        }
        
        // TODO: Add statement to program's statement list
        // For now, just free the statement
        mjs_ast_free(stmt);
    }
    
    return program;
}

/* AST node management */
void mjs_ast_node_free(mjs_ast_node_t* node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_LITERAL_NUMBER:
        case AST_LITERAL_STRING:
        case AST_LITERAL_BOOLEAN:
        case AST_LITERAL_NULL:
        case AST_LITERAL_UNDEFINED:
            if (node->u.literal.value.tag == MJS_TAG_STRING && 
                node->u.literal.value.u.string) {
                MJS_FREE(node->u.literal.value.u.string->data);
                MJS_FREE(node->u.literal.value.u.string);
            }
            break;
            
        case AST_IDENTIFIER:
            if (node->u.identifier.name) {
                MJS_FREE(node->u.identifier.name);
            }
            break;
            
        case AST_BINARY_EXPRESSION:
            mjs_ast_node_free(node->u.binary.left);
        mjs_ast_node_free(node->u.binary.right);
            break;
            
        case AST_UNARY_EXPRESSION:
            mjs_ast_node_free(node->u.unary.argument);
            break;
            
        case AST_MEMBER_EXPRESSION:
            mjs_ast_node_free(node->u.member.object);
        mjs_ast_node_free(node->u.member.property);
            break;
            
        case AST_CALL_EXPRESSION:
            mjs_ast_node_free(node->u.call.callee);
        if (node->u.call.arguments) {
            for (size_t i = 0; i < node->u.call.argument_count; i++) {
                mjs_ast_node_free(node->u.call.arguments[i]);
                }
                MJS_FREE(node->u.call.arguments);
            }
            break;
            
        case AST_EXPRESSION_STATEMENT:
            mjs_ast_node_free(node->u.expr_stmt.expression);
            break;
            
        case AST_BLOCK_STATEMENT:
            if (node->u.block.statements) {
            for (size_t i = 0; i < node->u.block.statement_count; i++) {
                mjs_ast_node_free(node->u.block.statements[i]);
                }
                MJS_FREE(node->u.block.statements);
            }
            break;
            
        case AST_FUNCTION_DECLARATION:
            MJS_FREE(node->u.function.name);
            if (node->u.function.params) {
                for (size_t i = 0; i < node->u.function.param_count; i++) {
                mjs_ast_node_free(node->u.function.params[i]);
                }
                MJS_FREE(node->u.function.params);
            }
            mjs_ast_node_free(node->u.function.body);
            break;
            
        case AST_PROGRAM:
            if (node->u.program.body) {
            for (size_t i = 0; i < node->u.program.statement_count; i++) {
                mjs_ast_node_free(node->u.program.body[i]);
                }
                MJS_FREE(node->u.program.body);
            }
            break;
            
        default:
            // Handle other node types as needed
            break;
    }
    
    MJS_FREE(node);
}

/* Alias for mjs_ast_node_free */
void mjs_ast_free(mjs_ast_node_t* node) {
    mjs_ast_node_free(node);
}

/* AST utility functions */
const char* mjs_ast_type_to_string(mjs_ast_type_t type) {
    switch (type) {
        case AST_LITERAL_NUMBER:
        case AST_LITERAL_STRING:
        case AST_LITERAL_BOOLEAN:
        case AST_LITERAL_NULL:
        case AST_LITERAL_UNDEFINED: return "Literal";
        case AST_IDENTIFIER: return "Identifier";
        case AST_BINARY_EXPRESSION: return "BinaryExpression";
        case AST_UNARY_EXPRESSION: return "UnaryExpression";
        case AST_ASSIGNMENT_EXPRESSION: return "AssignmentExpression";
        case AST_UPDATE_EXPRESSION: return "UpdateExpression";
        case AST_LOGICAL_EXPRESSION: return "LogicalExpression";
        case AST_CONDITIONAL_EXPRESSION: return "ConditionalExpression";
        case AST_CALL_EXPRESSION: return "CallExpression";
        case AST_MEMBER_EXPRESSION: return "MemberExpression";
        case AST_ARRAY_EXPRESSION: return "ArrayLiteral";
        case AST_OBJECT_EXPRESSION: return "ObjectLiteral";
        case AST_FUNCTION_EXPRESSION: return "FunctionExpression";
        case AST_ARROW_FUNCTION_EXPRESSION: return "ArrowFunction";
        case AST_EXPRESSION_STATEMENT: return "ExpressionStatement";
        case AST_BLOCK_STATEMENT: return "BlockStatement";
        case AST_VARIABLE_DECLARATION: return "VariableDeclaration";
        case AST_FUNCTION_DECLARATION: return "FunctionDeclaration";
        case AST_RETURN_STATEMENT: return "ReturnStatement";
        case AST_IF_STATEMENT: return "IfStatement";
        case AST_WHILE_STATEMENT: return "WhileStatement";
        case AST_FOR_STATEMENT: return "ForStatement";
        case AST_BREAK_STATEMENT: return "BreakStatement";
        case AST_CONTINUE_STATEMENT: return "ContinueStatement";
        case AST_PROGRAM: return "Program";
        default: return "Unknown";
    }
}