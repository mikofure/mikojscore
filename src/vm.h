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
 * MikoJSCore Virtual Machine
 * Stack-based bytecode interpreter for JavaScript execution
 */

#ifndef MIKOJS_VM_H
#define MIKOJS_VM_H

#include "mikojs_internal.h"
#include "parser.h"

/* Bytecode opcodes */
typedef enum {
    /* Basic operations */
    OP_NOP,
    
    /* Stack operations */
    OP_PUSH_UNDEFINED,
    OP_PUSH_NULL,
    OP_PUSH_TRUE,
    OP_PUSH_FALSE,
    OP_PUSH_NUMBER,
    OP_PUSH_STRING,
    OP_PUSH_BIGINT,
    OP_POP,
    OP_DUP,
    OP_SWAP,
    
    /* Variable operations */
    OP_LOAD_CONST,
    OP_LOAD_VAR,
    OP_STORE_VAR,
    OP_LOAD_LOCAL,
    OP_STORE_LOCAL,
    OP_LOAD_GLOBAL,
    OP_STORE_GLOBAL,
    OP_DECLARE_VAR,
    OP_DECLARE_LET,
    OP_DECLARE_CONST,
    
    /* Arithmetic operations */
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_NEG,
    OP_PLUS,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_MODULO,
    OP_EXPONENT,
    OP_NEGATE,
    OP_INCREMENT,
    OP_DECREMENT,
    
    /* Comparison operations */
    OP_EQ,
    OP_NE,
    OP_EQUAL,
    OP_NOT_EQUAL,
    OP_STRICT_EQUAL,
    OP_STRICT_NOT_EQUAL,
    OP_LESS_THAN,
    OP_LESS_EQUAL,
    OP_GREATER_THAN,
    OP_GREATER_EQUAL,
    OP_LT,
    OP_LE,
    OP_GT,
    OP_GE,
    
    /* Logical operations */
    OP_AND,
    OP_OR,
    OP_NOT,
    OP_LOGICAL_AND,
    OP_LOGICAL_OR,
    OP_LOGICAL_NOT,
    OP_NULLISH_COALESCING,
    
    /* Bitwise operations */
    OP_BIT_AND,
    OP_BIT_OR,
    OP_BIT_XOR,
    OP_BIT_NOT,
    OP_SHL,
    OP_SHR,
    OP_BITWISE_AND,
    OP_BITWISE_OR,
    OP_BITWISE_XOR,
    OP_BITWISE_NOT,
    OP_LEFT_SHIFT,
    OP_RIGHT_SHIFT,
    OP_UNSIGNED_RIGHT_SHIFT,
    
    /* Object operations */
    OP_NEW_OBJECT,
    OP_NEW_ARRAY,
    OP_GET_PROP,
    OP_SET_PROP,
    OP_GET_PROPERTY,
    OP_SET_PROPERTY,
    OP_GET_PROP_COMPUTED,
    OP_SET_PROP_COMPUTED,
    OP_DELETE_PROPERTY,
    OP_HAS_PROPERTY,
    OP_GET_ELEMENT,
    OP_SET_ELEMENT,
    OP_TYPEOF,
    OP_INSTANCEOF,
    
    /* Array operations */
    OP_ARRAY_PUSH,
    OP_ARRAY_POP,
    OP_ARRAY_GET,
    OP_ARRAY_SET,
    OP_ARRAY_LENGTH,
    
    /* Function operations */
    OP_CALL,
    OP_NEW,
    OP_RETURN,
    OP_YIELD,
    OP_AWAIT,
    
    /* Control flow */
    OP_JUMP,
    OP_JUMP_IF_TRUE,
    OP_JUMP_IF_FALSE,
    OP_JUMP_IF_NULL_OR_UNDEFINED,
    
    /* Exception handling */
    OP_THROW,
    OP_TRY_BEGIN,
    OP_TRY_END,
    OP_CATCH_BEGIN,
    OP_CATCH_END,
    OP_FINALLY_BEGIN,
    OP_FINALLY_END,
    
    /* Special */
    OP_DEBUGGER,
    OP_HALT
} mjs_opcode_t;

/* Bytecode instruction */
typedef struct {
    mjs_opcode_t opcode;
    union {
        int32_t i32;
        uint32_t u32;
        double number;
        void* ptr;
    } operand;
} mjs_instruction_t;

/* Bytecode structure */
struct mjs_bytecode {
    mjs_instruction_t* instructions;
    size_t instruction_count;
    size_t instruction_capacity;
    
    /* Constant pool */
    mjs_value_t* constants;
    size_t constant_count;
    size_t constant_capacity;
    
    /* String pool */
    mjs_string_t** strings;
    size_t string_count;
    size_t string_capacity;
    
    /* Debug information */
    struct {
        size_t line;
        size_t column;
    }* debug_info;
    
    /* Function metadata */
    char* name;
    size_t param_count;
    size_t local_count;
    bool is_async;
    bool is_generator;
};

/* Call frame */
typedef struct mjs_call_frame {
    mjs_bytecode_t* bytecode;
    size_t pc; /* program counter */
    mjs_value_t* locals;
    size_t local_count;
    size_t locals_base;
    mjs_value_t this_value;
    struct mjs_call_frame* previous;
} mjs_call_frame_t;

/* Exception handler */
typedef struct mjs_exception_handler {
    size_t try_start;
    size_t try_end;
    size_t catch_start;
    size_t finally_start;
    struct mjs_exception_handler* next;
} mjs_exception_handler_t;

/* Virtual machine structure */
struct mjs_vm {
    mjs_context_t* context;
    int state;
    
    /* Execution stack */
    mjs_value_t* stack;
    size_t stack_size;
    size_t stack_capacity;
    size_t stack_top;
    
    /* Call stack */
    mjs_call_frame_t* call_stack;
    size_t call_stack_top;
    size_t call_stack_capacity;
    
    /* Exception handling */
    mjs_exception_handler_t* exception_stack;
    size_t exception_stack_top;
    size_t exception_stack_capacity;
    
    /* VM statistics */
    struct {
        size_t instructions_executed;
    } stats;
    size_t call_depth;
    size_t max_call_depth;
    
    /* Exception handling */
    mjs_exception_handler_t* exception_handlers;
    mjs_value_t exception_value;
    bool has_exception;
    
    /* Execution state */
    bool running;
    bool halted;
    
    /* Performance counters */
    size_t instruction_count;
    size_t gc_count;
};

/* VM functions */
mjs_vm_t* mjs_vm_new(mjs_context_t* ctx);
void mjs_vm_free(mjs_vm_t* vm);

mjs_result_t mjs_vm_execute(mjs_vm_t* vm, mjs_bytecode_t* bytecode, mjs_value_t* result);
mjs_result_t mjs_vm_call_function(mjs_vm_t* vm, mjs_function_t* func, mjs_value_t this_arg, int argc, mjs_value_t* argv, mjs_value_t* result);

/* Stack operations */
void mjs_vm_push(mjs_vm_t* vm, mjs_value_t value);
mjs_value_t mjs_vm_pop(mjs_vm_t* vm);
mjs_value_t mjs_vm_peek(mjs_vm_t* vm, size_t offset);
void mjs_vm_ensure_stack_space(mjs_vm_t* vm, size_t needed);

/* Call frame management */
mjs_call_frame_t* mjs_vm_push_frame(mjs_vm_t* vm, mjs_bytecode_t* bytecode, mjs_value_t this_value);
void mjs_vm_pop_frame(mjs_vm_t* vm);

/* Exception handling */
void mjs_vm_throw_exception(mjs_vm_t* vm, mjs_value_t exception);
bool mjs_vm_has_exception(mjs_vm_t* vm);
mjs_value_t mjs_vm_get_exception(mjs_vm_t* vm);
void mjs_vm_clear_exception(mjs_vm_t* vm);

/* Bytecode functions */
mjs_bytecode_t* mjs_bytecode_new(void);
void mjs_bytecode_free(mjs_bytecode_t* bytecode);

bool mjs_bytecode_emit(mjs_bytecode_t* bytecode, mjs_instruction_t instruction);
void mjs_bytecode_emit_with_operand(mjs_bytecode_t* bytecode, mjs_opcode_t opcode, int32_t operand);
void mjs_bytecode_emit_constant(mjs_bytecode_t* bytecode, mjs_value_t value);
void mjs_bytecode_emit_string(mjs_bytecode_t* bytecode, const char* str);

uint32_t mjs_bytecode_add_constant(mjs_bytecode_t* bytecode, mjs_value_t value);
uint32_t mjs_bytecode_add_string(mjs_bytecode_t* bytecode, const char* str);

void mjs_bytecode_patch_jump(mjs_bytecode_t* bytecode, size_t jump_addr);
size_t mjs_bytecode_get_current_offset(mjs_bytecode_t* bytecode);
size_t mjs_bytecode_emit_jump(mjs_bytecode_t* bytecode, mjs_opcode_t opcode, uint32_t placeholder);

/* Compiler functions */
mjs_bytecode_t* mjs_compile_ast(mjs_context_t* ctx, mjs_ast_node_t* ast);
mjs_result_t mjs_compile_expression(mjs_context_t* ctx, mjs_ast_node_t* expr, mjs_bytecode_t* bytecode);
mjs_result_t mjs_compile_statement(mjs_context_t* ctx, mjs_ast_node_t* stmt, mjs_bytecode_t* bytecode);

/* Debug functions */
void mjs_bytecode_disassemble(mjs_bytecode_t* bytecode);
void mjs_vm_dump_stack(mjs_vm_t* vm);
const char* mjs_opcode_to_string(mjs_opcode_t opcode);

#endif /* MIKOJS_VM_H */