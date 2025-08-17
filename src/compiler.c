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
 * MikoJSCore Bytecode Compiler
 * Compiles AST nodes into bytecode instructions for the VM
 */

#include "vm.h"
#include "parser.h"
#include "mikojs_internal.h"

/* Compiler context for tracking compilation state */
typedef struct {
    mjs_bytecode_t* bytecode;
    mjs_context_t* context;
    size_t current_scope_depth;
    bool has_error;
    char error_message[256];
} mjs_compiler_t;

/* Forward declarations */
static bool compile_expression(mjs_compiler_t* compiler, mjs_ast_node_t* node);
static bool compile_statement(mjs_compiler_t* compiler, mjs_ast_node_t* node);
static bool compile_literal(mjs_compiler_t* compiler, mjs_ast_node_t* node);
static bool compile_identifier(mjs_compiler_t* compiler, mjs_ast_node_t* node);
static bool compile_binary_expression(mjs_compiler_t* compiler, mjs_ast_node_t* node);
static bool compile_unary_expression(mjs_compiler_t* compiler, mjs_ast_node_t* node);
static bool compile_call_expression(mjs_compiler_t* compiler, mjs_ast_node_t* node);
static bool compile_member_expression(mjs_compiler_t* compiler, mjs_ast_node_t* node);
static bool compile_assignment_expression(mjs_compiler_t* compiler, mjs_ast_node_t* node);
static bool compile_function_declaration(mjs_compiler_t* compiler, mjs_ast_node_t* node);
static bool compile_variable_declaration(mjs_compiler_t* compiler, mjs_ast_node_t* node);
static bool compile_block_statement(mjs_compiler_t* compiler, mjs_ast_node_t* node);
static bool compile_if_statement(mjs_compiler_t* compiler, mjs_ast_node_t* node);
static bool compile_while_statement(mjs_compiler_t* compiler, mjs_ast_node_t* node);
static bool compile_return_statement(mjs_compiler_t* compiler, mjs_ast_node_t* node);

/* Error handling */
static void compiler_error(mjs_compiler_t* compiler, const char* message) {
    if (!compiler || compiler->has_error) return;
    
    compiler->has_error = true;
    snprintf(compiler->error_message, sizeof(compiler->error_message), 
             "Compilation error: %s", message);
}

/* Bytecode generation helpers */
static bool emit_instruction(mjs_compiler_t* compiler, mjs_opcode_t opcode, uint32_t operand) {
    if (!compiler || !compiler->bytecode) return false;
    
    mjs_instruction_t instruction;
    instruction.opcode = opcode;
    instruction.operand.u32 = operand;
    
    return mjs_bytecode_emit(compiler->bytecode, instruction);
}

static uint32_t add_constant(mjs_compiler_t* compiler, mjs_value_t value) {
    if (!compiler || !compiler->bytecode) return 0;
    
    return mjs_bytecode_add_constant(compiler->bytecode, value);
}

static uint32_t add_string(mjs_compiler_t* compiler, const char* str) {
    if (!compiler || !compiler->bytecode || !str) return 0;
    
    return mjs_bytecode_add_string(compiler->bytecode, str);
}

/* Main compilation functions */
mjs_bytecode_t* mjs_compile_ast(mjs_context_t* ctx, mjs_ast_node_t* ast) {
    if (!ctx || !ast) return NULL;
    
    mjs_compiler_t compiler;
    memset(&compiler, 0, sizeof(compiler));
    
    compiler.bytecode = mjs_bytecode_new();
    if (!compiler.bytecode) return NULL;
    
    compiler.context = ctx;
    compiler.current_scope_depth = 0;
    compiler.has_error = false;
    
    // Compile the AST
    bool success = false;
    switch (ast->type) {
        case AST_PROGRAM:
            success = compile_statement(&compiler, ast);
            break;
        default:
            success = compile_expression(&compiler, ast);
            break;
    }
    
    if (!success || compiler.has_error) {
        mjs_bytecode_free(compiler.bytecode);
        return NULL;
    }
    
    // Add final return instruction if not present
    emit_instruction(&compiler, OP_RETURN, 0);
    
    return compiler.bytecode;
}

/* Expression compilation */
static bool compile_expression(mjs_compiler_t* compiler, mjs_ast_node_t* node) {
    if (!compiler || !node) return false;
    
    switch (node->type) {
        case AST_LITERAL_NUMBER:
        case AST_LITERAL_STRING:
        case AST_LITERAL_BOOLEAN:
        case AST_LITERAL_NULL:
        case AST_LITERAL_UNDEFINED:
            return compile_literal(compiler, node);
            
        case AST_IDENTIFIER:
            return compile_identifier(compiler, node);
            
        case AST_BINARY_EXPRESSION:
            return compile_binary_expression(compiler, node);
            
        case AST_UNARY_EXPRESSION:
            return compile_unary_expression(compiler, node);
            
        case AST_ASSIGNMENT_EXPRESSION:
            return compile_assignment_expression(compiler, node);
            
        case AST_CALL_EXPRESSION:
            return compile_call_expression(compiler, node);
            
        case AST_MEMBER_EXPRESSION:
            return compile_member_expression(compiler, node);
            
        case AST_ARRAY_EXPRESSION: {
            // Create empty array and push elements
            emit_instruction(compiler, OP_NEW_ARRAY, 0);
            
            if (node->u.array.elements) {
                for (size_t i = 0; i < node->u.array.element_count; i++) {
                    if (!compile_expression(compiler, node->u.array.elements[i])) {
                        return false;
                    }
                    emit_instruction(compiler, OP_ARRAY_PUSH, 0);
                }
            }
            return true;
        }
        
        case AST_OBJECT_EXPRESSION: {
            // Create empty object and set properties
            emit_instruction(compiler, OP_NEW_OBJECT, 0);
            
            if (node->u.object.properties) {
                for (size_t i = 0; i < node->u.object.property_count; i++) {
                    // TODO: Compile object properties
                    // For now, just create empty object
                }
            }
            return true;
        }
        
        case AST_FUNCTION_EXPRESSION: {
            // TODO: Implement function expression compilation
            compiler_error(compiler, "Function expressions not yet implemented");
            return false;
        }
        
        default:
            compiler_error(compiler, "Unknown expression type");
            return false;
    }
}

/* Statement compilation */
static bool compile_statement(mjs_compiler_t* compiler, mjs_ast_node_t* node) {
    if (!compiler || !node) return false;
    
    switch (node->type) {
        case AST_PROGRAM: {
            if (node->u.program.body) {
                for (size_t i = 0; i < node->u.program.statement_count; i++) {
                    if (!compile_statement(compiler, node->u.program.body[i])) {
                        return false;
                    }
                }
            }
            return true;
        }
        
        case AST_EXPRESSION_STATEMENT:
            if (!compile_expression(compiler, node->u.expr_stmt.expression)) {
                return false;
            }
            // Pop the expression result since it's not used
            emit_instruction(compiler, OP_POP, 0);
            return true;
            
        case AST_BLOCK_STATEMENT:
            return compile_block_statement(compiler, node);
            
        case AST_VARIABLE_DECLARATION:
            return compile_variable_declaration(compiler, node);
            
        case AST_FUNCTION_DECLARATION:
            return compile_function_declaration(compiler, node);
            
        case AST_IF_STATEMENT:
            return compile_if_statement(compiler, node);
            
        case AST_WHILE_STATEMENT:
            return compile_while_statement(compiler, node);
            
        case AST_RETURN_STATEMENT:
            return compile_return_statement(compiler, node);
            
        case AST_BREAK_STATEMENT:
            // TODO: Implement break statement with proper loop context
            return true;
            
        case AST_CONTINUE_STATEMENT:
            // TODO: Implement continue statement with proper loop context
            return true;
            
        default:
            compiler_error(compiler, "Unknown statement type");
            return false;
    }
}

/* Literal compilation */
static bool compile_literal(mjs_compiler_t* compiler, mjs_ast_node_t* node) {
    if (!compiler || !node) return false;
    
    mjs_value_t value;
    
    switch (node->type) {
        case AST_LITERAL_NUMBER:
            return emit_instruction(compiler, OP_LOAD_CONST, 
                add_constant(compiler, mjs_value_number(node->u.literal.value.u.number)));
        case AST_LITERAL_STRING:
            return emit_instruction(compiler, OP_LOAD_CONST,
                add_string(compiler, node->u.literal.value.u.string->data));
        case AST_LITERAL_BOOLEAN:
            return emit_instruction(compiler, 
                node->u.literal.value.u.boolean ? OP_PUSH_TRUE : OP_PUSH_FALSE, 0);
        case AST_LITERAL_NULL:
            return emit_instruction(compiler, OP_PUSH_NULL, 0);
        case AST_LITERAL_UNDEFINED:
            return emit_instruction(compiler, OP_PUSH_UNDEFINED, 0);

        default:
            compiler_error(compiler, "Unknown literal type");
            return false;
    }
}

/* Identifier compilation */
static bool compile_identifier(mjs_compiler_t* compiler, mjs_ast_node_t* node) {
    if (!compiler || !node || node->type != AST_IDENTIFIER) return false;
    
    const char* name = node->u.identifier.name;
    if (!name) return false;
    
    uint32_t string_index = add_string(compiler, name);
    return emit_instruction(compiler, OP_LOAD_VAR, string_index);
}

/* Binary expression compilation */
static bool compile_binary_expression(mjs_compiler_t* compiler, mjs_ast_node_t* node) {
    if (!compiler || !node || node->type != AST_BINARY_EXPRESSION) return false;
    
    // Compile left operand
    if (!compile_expression(compiler, node->u.binary.left)) {
        return false;
    }
    
    // Compile right operand
    if (!compile_expression(compiler, node->u.binary.right)) {
        return false;
    }
    
    mjs_token_type_t op = node->u.binary.operator;
    
    switch (op) {
        case TOKEN_PLUS:
            return emit_instruction(compiler, OP_ADD, 0);
        case TOKEN_MINUS:
            return emit_instruction(compiler, OP_SUB, 0);
        case TOKEN_MULTIPLY:
            return emit_instruction(compiler, OP_MUL, 0);
        case TOKEN_DIVIDE:
            return emit_instruction(compiler, OP_DIV, 0);
        case TOKEN_MODULO:
            return emit_instruction(compiler, OP_MOD, 0);
        case TOKEN_EQUAL:
            return emit_instruction(compiler, OP_EQ, 0);
        case TOKEN_NOT_EQUAL:
            return emit_instruction(compiler, OP_NE, 0);
        case TOKEN_LESS_THAN:
            return emit_instruction(compiler, OP_LT, 0);
        case TOKEN_LESS_EQUAL:
            return emit_instruction(compiler, OP_LE, 0);
        case TOKEN_GREATER_THAN:
            return emit_instruction(compiler, OP_GT, 0);
        case TOKEN_GREATER_EQUAL:
            return emit_instruction(compiler, OP_GE, 0);
        case TOKEN_LOGICAL_AND:
            return emit_instruction(compiler, OP_AND, 0);
        case TOKEN_LOGICAL_OR:
            return emit_instruction(compiler, OP_OR, 0);
        case TOKEN_BITWISE_AND:
            return emit_instruction(compiler, OP_BIT_AND, 0);
        case TOKEN_BITWISE_OR:
            return emit_instruction(compiler, OP_BIT_OR, 0);
        case TOKEN_BITWISE_XOR:
            return emit_instruction(compiler, OP_BIT_XOR, 0);
        case TOKEN_LEFT_SHIFT:
            return emit_instruction(compiler, OP_SHL, 0);
        case TOKEN_RIGHT_SHIFT:
            return emit_instruction(compiler, OP_SHR, 0);
        default:
            compiler_error(compiler, "Unknown binary operator");
            return false;
    }
}

/* Unary expression compilation */
static bool compile_unary_expression(mjs_compiler_t* compiler, mjs_ast_node_t* node) {
    if (!compiler || !node || node->type != AST_UNARY_EXPRESSION) return false;
    
    mjs_token_type_t op = node->u.unary.operator;
    
    // For prefix operators, compile operand first
    if (node->u.unary.prefix) {
        if (!compile_expression(compiler, node->u.unary.argument)) {
            return false;
        }
        
        switch (op) {
            case TOKEN_PLUS:
                return emit_instruction(compiler, OP_PLUS, 0);
            case TOKEN_MINUS:
                return emit_instruction(compiler, OP_NEG, 0);
            case TOKEN_LOGICAL_NOT:
                return emit_instruction(compiler, OP_NOT, 0);
            case TOKEN_BITWISE_NOT:
                return emit_instruction(compiler, OP_BIT_NOT, 0);
            case TOKEN_KEYWORD_TYPEOF:
                return emit_instruction(compiler, OP_TYPEOF, 0);
            case TOKEN_KEYWORD_VOID:
                emit_instruction(compiler, OP_POP, 0); // Discard value
                uint32_t undef_index = add_constant(compiler, mjs_value_undefined());
                return emit_instruction(compiler, OP_LOAD_CONST, undef_index);
            default:
                compiler_error(compiler, "Unknown unary operator");
                return false;
        }
    } else {
        // Postfix operators (++ and --)
        // TODO: Implement postfix increment/decrement
        compiler_error(compiler, "Postfix operators not yet implemented");
        return false;
    }
}

/* Assignment expression compilation */
static bool compile_assignment_expression(mjs_compiler_t* compiler, mjs_ast_node_t* node) {
    if (!compiler || !node || node->type != AST_ASSIGNMENT_EXPRESSION) return false;
    
    // Compile right-hand side first
    if (!compile_expression(compiler, node->u.assignment.right)) {
        return false;
    }
    
    // Handle different assignment targets
    mjs_ast_node_t* left = node->u.assignment.left;
    
    if (left->type == AST_IDENTIFIER) {
        // Simple variable assignment
        uint32_t string_index = add_string(compiler, left->u.identifier.name);
        return emit_instruction(compiler, OP_STORE_VAR, string_index);
    } else if (left->type == AST_MEMBER_EXPRESSION) {
        // Property assignment
        if (!compile_expression(compiler, left->u.member.object)) {
            return false;
        }
        
        if (left->u.member.computed) {
            // obj[prop] = value
            if (!compile_expression(compiler, left->u.member.property)) {
                return false;
            }
            return emit_instruction(compiler, OP_SET_PROP_COMPUTED, 0);
        } else {
            // obj.prop = value
            if (left->u.member.property->type == AST_IDENTIFIER) {
                uint32_t prop_index = add_string(compiler, 
                    left->u.member.property->u.identifier.name);
                return emit_instruction(compiler, OP_SET_PROP, prop_index);
            }
        }
    }
    
    compiler_error(compiler, "Invalid assignment target");
    return false;
}

/* Call expression compilation */
static bool compile_call_expression(mjs_compiler_t* compiler, mjs_ast_node_t* node) {
    if (!compiler || !node || node->type != AST_CALL_EXPRESSION) return false;
    
    // Compile callee
    if (!compile_expression(compiler, node->u.call.callee)) {
        return false;
    }
    
    // Compile arguments
    size_t arg_count = node->u.call.argument_count;
    if (node->u.call.arguments) {
        for (size_t i = 0; i < arg_count; i++) {
            if (!compile_expression(compiler, node->u.call.arguments[i])) {
                return false;
            }
        }
    }
    
    return emit_instruction(compiler, OP_CALL, (uint32_t)arg_count);
}

/* Member expression compilation */
static bool compile_member_expression(mjs_compiler_t* compiler, mjs_ast_node_t* node) {
    if (!compiler || !node || node->type != AST_MEMBER_EXPRESSION) return false;
    
    // Compile object
    if (!compile_expression(compiler, node->u.member.object)) {
        return false;
    }
    
    if (node->u.member.computed) {
        // obj[prop]
        if (!compile_expression(compiler, node->u.member.property)) {
            return false;
        }
        return emit_instruction(compiler, OP_GET_PROP_COMPUTED, 0);
    } else {
        // obj.prop
        if (node->u.member.property->type == AST_IDENTIFIER) {
            uint32_t prop_index = add_string(compiler, 
                node->u.member.property->u.identifier.name);
            return emit_instruction(compiler, OP_GET_PROP, prop_index);
        }
    }
    
    compiler_error(compiler, "Invalid member expression");
    return false;
}

/* Variable declaration compilation */
static bool compile_variable_declaration(mjs_compiler_t* compiler, mjs_ast_node_t* node) {
    if (!compiler || !node || node->type != AST_VARIABLE_DECLARATION) return false;
    
    // Compile all variable declarators
    if (node->u.var_decl.declarations) {
        for (size_t i = 0; i < node->u.var_decl.declaration_count; i++) {
            // Compile initializer if present
            if (node->u.var_decl.declarations[i].init) {
                if (!compile_expression(compiler, node->u.var_decl.declarations[i].init)) {
                    return false;
                }
            } else {
                // No initializer, load undefined
                emit_instruction(compiler, OP_PUSH_UNDEFINED, 0);
            }
            
            // Store in variable
            uint32_t name_index = add_string(compiler, 
                node->u.var_decl.declarations[i].id->u.identifier.name);
            if (!emit_instruction(compiler, OP_STORE_VAR, name_index)) {
                return false;
            }
        }
    }
    
    return true;
}

/* Function declaration compilation */
static bool compile_function_declaration(mjs_compiler_t* compiler, mjs_ast_node_t* node) {
    if (!compiler || !node || node->type != AST_FUNCTION_DECLARATION) return false;
    
    // Create function name string
    uint32_t name_index = add_string(compiler, node->u.function.name);
    
    // TODO: Implement function compilation
    // For now, just emit a placeholder
    return emit_instruction(compiler, OP_PUSH_UNDEFINED, 0);
}

/* Block statement compilation */
static bool compile_block_statement(mjs_compiler_t* compiler, mjs_ast_node_t* node) {
    if (!compiler || !node || node->type != AST_BLOCK_STATEMENT) return false;
    
    // Enter new scope
    compiler->current_scope_depth++;
    
    // Compile all statements in the block
    if (node->u.block.statements) {
        for (size_t i = 0; i < node->u.block.statement_count; i++) {
            if (!compile_statement(compiler, node->u.block.statements[i])) {
                return false;
            }
        }
    }
    
    // Exit scope
    compiler->current_scope_depth--;
    
    return true;
}

/* Control flow statement compilation */
static bool compile_if_statement(mjs_compiler_t* compiler, mjs_ast_node_t* node) {
    if (!compiler || !node || node->type != AST_IF_STATEMENT) return false;
    
    // Compile condition
    if (!compile_expression(compiler, node->u.if_stmt.test)) {
        return false;
    }
    
    // Jump if false
    size_t else_jump = mjs_bytecode_emit_jump(compiler->bytecode, OP_JUMP_IF_FALSE, 0);
    
    // Compile consequent
    if (!compile_statement(compiler, node->u.if_stmt.consequent)) {
        return false;
    }
    
    if (node->u.if_stmt.alternate) {
        // Jump over else block
        size_t end_jump = mjs_bytecode_emit_jump(compiler->bytecode, OP_JUMP, 0);
        
        // Patch else jump
        mjs_bytecode_patch_jump(compiler->bytecode, else_jump);
        
        // Compile alternate
        if (!compile_statement(compiler, node->u.if_stmt.alternate)) {
            return false;
        }
        
        // Patch end jump
        mjs_bytecode_patch_jump(compiler->bytecode, end_jump);
    } else {
        // Patch else jump
        mjs_bytecode_patch_jump(compiler->bytecode, else_jump);
    }
    
    return true;
}

static bool compile_while_statement(mjs_compiler_t* compiler, mjs_ast_node_t* node) {
    if (!compiler || !node || node->type != AST_WHILE_STATEMENT) return false;
    
    // Mark loop start
    size_t loop_start = mjs_bytecode_get_current_offset(compiler->bytecode);
    
    // Compile condition
    if (!compile_expression(compiler, node->u.while_stmt.test)) {
        return false;
    }
    
    // Jump if false (exit loop)
    size_t exit_jump = mjs_bytecode_emit_jump(compiler->bytecode, OP_JUMP_IF_FALSE, 0);
    
    // Compile body
    if (!compile_statement(compiler, node->u.while_stmt.body)) {
        return false;
    }
    
    // Jump back to condition
    emit_instruction(compiler, OP_JUMP, (uint32_t)loop_start);
    
    // Patch exit jump
    mjs_bytecode_patch_jump(compiler->bytecode, exit_jump);
    
    return true;
}

static bool compile_return_statement(mjs_compiler_t* compiler, mjs_ast_node_t* node) {
    if (!compiler || !node || node->type != AST_RETURN_STATEMENT) return false;
    
    if (node->u.return_stmt.argument) {
        // Compile return value
        if (!compile_expression(compiler, node->u.return_stmt.argument)) {
            return false;
        }
    } else {
        // Return undefined
        uint32_t undef_index = add_constant(compiler, mjs_value_undefined());
        emit_instruction(compiler, OP_LOAD_CONST, undef_index);
    }
    
    return emit_instruction(compiler, OP_RETURN, 0);
}

/* Utility functions */
const char* mjs_compiler_get_error(mjs_compiler_t* compiler) {
    return compiler ? compiler->error_message : "Invalid compiler";
}