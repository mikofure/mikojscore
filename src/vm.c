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
 * Stack-based virtual machine for executing bytecode
 */

#include "vm.h"
#include "mikojs_internal.h"
#include "gc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* VM constants */
#define VM_STACK_SIZE 1024
#define VM_CALL_STACK_SIZE 256
#define VM_EXCEPTION_STACK_SIZE 64

/* Forward declarations */
static bool vm_execute_instruction(mjs_vm_t* vm, mjs_instruction_t* instr);
static bool vm_push(mjs_vm_t* vm, mjs_value_t value);
static mjs_value_t vm_pop(mjs_vm_t* vm);
static mjs_value_t vm_peek(mjs_vm_t* vm, size_t offset);
static void vm_drop(mjs_vm_t* vm, size_t count);
static bool vm_push_frame(mjs_vm_t* vm, mjs_bytecode_t* bytecode, size_t locals_count);
static bool vm_pop_frame(mjs_vm_t* vm);
static mjs_call_frame_t* vm_current_frame(mjs_vm_t* vm);
static mjs_value_t vm_add(mjs_vm_t* vm, mjs_value_t a, mjs_value_t b);
static mjs_value_t vm_subtract(mjs_vm_t* vm, mjs_value_t a, mjs_value_t b);
static mjs_value_t vm_multiply(mjs_vm_t* vm, mjs_value_t a, mjs_value_t b);
static mjs_value_t vm_divide(mjs_vm_t* vm, mjs_value_t a, mjs_value_t b);
static mjs_value_t vm_modulo(mjs_vm_t* vm, mjs_value_t a, mjs_value_t b);
static mjs_value_t vm_negate(mjs_vm_t* vm, mjs_value_t a);
static mjs_value_t vm_unary_plus(mjs_vm_t* vm, mjs_value_t a);
static bool vm_equals(mjs_vm_t* vm, mjs_value_t a, mjs_value_t b);
static bool vm_less_than(mjs_vm_t* vm, mjs_value_t a, mjs_value_t b);
static bool vm_less_than_or_equal(mjs_vm_t* vm, mjs_value_t a, mjs_value_t b);
static bool vm_greater_than(mjs_vm_t* vm, mjs_value_t a, mjs_value_t b);
static bool vm_greater_than_or_equal(mjs_vm_t* vm, mjs_value_t a, mjs_value_t b);
static const char* vm_typeof(mjs_value_t value);

/* VM creation and destruction */
mjs_vm_t* mjs_vm_new(mjs_context_t* ctx) {
    if (!ctx) return NULL;
    
    mjs_vm_t* vm = (mjs_vm_t*)malloc(sizeof(mjs_vm_t));
    if (!vm) return NULL;
    
    memset(vm, 0, sizeof(mjs_vm_t));
    
    vm->context = ctx;
    vm->stack_capacity = VM_STACK_SIZE;
    vm->call_stack_capacity = VM_CALL_STACK_SIZE;
    vm->exception_stack_capacity = VM_EXCEPTION_STACK_SIZE;
    
    // Allocate stacks
    vm->stack = (mjs_value_t*)malloc(sizeof(mjs_value_t) * vm->stack_capacity);
    vm->call_stack = (mjs_call_frame_t*)malloc(sizeof(mjs_call_frame_t) * vm->call_stack_capacity);
    vm->exception_stack = (mjs_exception_handler_t*)malloc(sizeof(mjs_exception_handler_t) * vm->exception_stack_capacity);
    
    if (!vm->stack || !vm->call_stack || !vm->exception_stack) {
        mjs_vm_free(vm);
        return NULL;
    }
    
    vm->stack_top = 0;
    vm->call_stack_top = 0;
    vm->exception_stack_top = 0;
    vm->state = VM_STATE_READY;
    
    return vm;
}

void mjs_vm_free(mjs_vm_t* vm) {
    if (!vm) return;
    
    free(vm->stack);
    free(vm->call_stack);
    free(vm->exception_stack);
    free(vm);
}

/* Stack operations */
static bool vm_push(mjs_vm_t* vm, mjs_value_t value) {
    if (!vm || vm->stack_top >= vm->stack_capacity) {
        return false;
    }
    
    vm->stack[vm->stack_top++] = value;
    return true;
}

static mjs_value_t vm_pop(mjs_vm_t* vm) {
    if (!vm || vm->stack_top == 0) {
        return mjs_value_undefined();
    }
    
    return vm->stack[--vm->stack_top];
}

static mjs_value_t vm_peek(mjs_vm_t* vm, size_t offset) {
    if (!vm || vm->stack_top <= offset) {
        return mjs_value_undefined();
    }
    
    return vm->stack[vm->stack_top - 1 - offset];
}

static void vm_drop(mjs_vm_t* vm, size_t count) {
    if (!vm) return;
    
    if (count > vm->stack_top) {
        vm->stack_top = 0;
    } else {
        vm->stack_top -= count;
    }
}

/* Call frame management */
static bool vm_push_frame(mjs_vm_t* vm, mjs_bytecode_t* bytecode, size_t locals_count) {
    if (!vm || vm->call_stack_top >= vm->call_stack_capacity) {
        return false;
    }
    
    mjs_call_frame_t* frame = &vm->call_stack[vm->call_stack_top++];
    frame->bytecode = bytecode;
    frame->pc = 0;
    frame->locals_base = vm->stack_top - locals_count;
    frame->this_value = mjs_value_undefined();
    
    return true;
}

static bool vm_pop_frame(mjs_vm_t* vm) {
    if (!vm || vm->call_stack_top == 0) {
        return false;
    }
    
    vm->call_stack_top--;
    return true;
}

static mjs_call_frame_t* vm_current_frame(mjs_vm_t* vm) {
    if (!vm || vm->call_stack_top == 0) {
        return NULL;
    }
    
    return &vm->call_stack[vm->call_stack_top - 1];
}

/* Bytecode execution */
mjs_result_t mjs_vm_execute(mjs_vm_t* vm, mjs_bytecode_t* bytecode, mjs_value_t* result) {
    if (!vm || !bytecode) {
        return MJS_ERROR;
    }
    
    // Push initial call frame
    if (!vm_push_frame(vm, bytecode, 0)) {
        return MJS_ERROR;
    }
    
    vm->state = VM_STATE_RUNNING;
    
    while (vm->state == VM_STATE_RUNNING && vm->call_stack_top > 0) {
        mjs_call_frame_t* frame = vm_current_frame(vm);
        if (!frame || frame->pc >= frame->bytecode->instruction_count) {
            vm_pop_frame(vm);
            continue;
        }
        
        mjs_instruction_t* instr = &frame->bytecode->instructions[frame->pc];
        frame->pc++;
        
        vm->stats.instructions_executed++;
        
        // Execute instruction
        if (!vm_execute_instruction(vm, instr)) {
            vm->state = VM_STATE_ERROR;
            return MJS_ERROR;
        }
    }
    
    vm->state = VM_STATE_READY;
    
    // Set result to top of stack if available
    if (result) {
        if (vm->stack_top > 0) {
            *result = vm_peek(vm, 0);
        } else {
            *result = mjs_value_undefined();
        }
    }
    
    return MJS_OK;
}

/* Instruction execution */
static bool vm_execute_instruction(mjs_vm_t* vm, mjs_instruction_t* instr) {
    if (!vm || !instr) return false;
    
    mjs_call_frame_t* frame = vm_current_frame(vm);
    if (!frame) return false;
    
    switch (instr->opcode) {
        case OP_NOP:
            break;
            
        case OP_LOAD_CONST: {
            if (instr->operand.u32 >= frame->bytecode->constant_count) {
                return false;
            }
            mjs_value_t value = frame->bytecode->constants[instr->operand.u32];
            return vm_push(vm, value);
        }
        
        case OP_LOAD_VAR: {
            if (instr->operand.u32 >= frame->bytecode->string_count) {
                return false;
            }
            const char* name = frame->bytecode->strings[instr->operand.u32]->data;
            
            // Look up variable in context
            mjs_value_t value;
            if (mjs_context_get_variable(vm->context, name, &value)) {
                return vm_push(vm, value);
            } else {
                // Variable not found, push undefined
                return vm_push(vm, mjs_value_undefined());
            }
        }
        
        case OP_STORE_VAR: {
            if (instr->operand.u32 >= frame->bytecode->string_count) {
                return false;
            }
            const char* name = frame->bytecode->strings[instr->operand.u32]->data;
            mjs_value_t value = vm_pop(vm);
            
            return mjs_context_set_variable(vm->context, name, value);
        }
        
        case OP_POP:
            vm_pop(vm);
            break;
            
        case OP_DUP: {
            mjs_value_t value = vm_peek(vm, 0);
            return vm_push(vm, value);
        }
        
        case OP_SWAP: {
            if (vm->stack_top < 2) return false;
            mjs_value_t a = vm_pop(vm);
            mjs_value_t b = vm_pop(vm);
            vm_push(vm, a);
            vm_push(vm, b);
            break;
        }
        
        // Arithmetic operations
        case OP_ADD: {
            mjs_value_t b = vm_pop(vm);
            mjs_value_t a = vm_pop(vm);
            mjs_value_t result = vm_add(vm, a, b);
            return vm_push(vm, result);
        }
        
        case OP_SUB: {
            mjs_value_t b = vm_pop(vm);
            mjs_value_t a = vm_pop(vm);
            mjs_value_t result = vm_subtract(vm, a, b);
            return vm_push(vm, result);
        }
        
        case OP_MUL: {
            mjs_value_t b = vm_pop(vm);
            mjs_value_t a = vm_pop(vm);
            mjs_value_t result = vm_multiply(vm, a, b);
            return vm_push(vm, result);
        }
        
        case OP_DIV: {
            mjs_value_t b = vm_pop(vm);
            mjs_value_t a = vm_pop(vm);
            mjs_value_t result = vm_divide(vm, a, b);
            return vm_push(vm, result);
        }
        
        case OP_MOD: {
            mjs_value_t b = vm_pop(vm);
            mjs_value_t a = vm_pop(vm);
            mjs_value_t result = vm_modulo(vm, a, b);
            return vm_push(vm, result);
        }
        
        case OP_NEG: {
            mjs_value_t a = vm_pop(vm);
            mjs_value_t result = vm_negate(vm, a);
            return vm_push(vm, result);
        }
        
        case OP_PLUS: {
            mjs_value_t a = vm_pop(vm);
            mjs_value_t result = vm_unary_plus(vm, a);
            return vm_push(vm, result);
        }
        
        // Comparison operations
        case OP_EQ: {
            mjs_value_t b = vm_pop(vm);
            mjs_value_t a = vm_pop(vm);
            bool result = vm_equals(vm, a, b);
            return vm_push(vm, mjs_value_boolean(result));
        }
        
        case OP_NE: {
            mjs_value_t b = vm_pop(vm);
            mjs_value_t a = vm_pop(vm);
            bool result = !vm_equals(vm, a, b);
            return vm_push(vm, mjs_value_boolean(result));
        }
        
        case OP_LT: {
            mjs_value_t b = vm_pop(vm);
            mjs_value_t a = vm_pop(vm);
            bool result = vm_less_than(vm, a, b);
            return vm_push(vm, mjs_value_boolean(result));
        }
        
        case OP_LE: {
            mjs_value_t b = vm_pop(vm);
            mjs_value_t a = vm_pop(vm);
            bool result = vm_less_than_or_equal(vm, a, b);
            return vm_push(vm, mjs_value_boolean(result));
        }
        
        case OP_GT: {
            mjs_value_t b = vm_pop(vm);
            mjs_value_t a = vm_pop(vm);
            bool result = vm_greater_than(vm, a, b);
            return vm_push(vm, mjs_value_boolean(result));
        }
        
        case OP_GE: {
            mjs_value_t b = vm_pop(vm);
            mjs_value_t a = vm_pop(vm);
            bool result = vm_greater_than_or_equal(vm, a, b);
            return vm_push(vm, mjs_value_boolean(result));
        }
        
        // Logical operations
        case OP_AND: {
            mjs_value_t b = vm_pop(vm);
            mjs_value_t a = vm_pop(vm);
            bool result = mjs_to_boolean(a) && mjs_to_boolean(b);
            return vm_push(vm, mjs_value_boolean(result));
        }
        
        case OP_OR: {
            mjs_value_t b = vm_pop(vm);
            mjs_value_t a = vm_pop(vm);
            bool result = mjs_to_boolean(a) || mjs_to_boolean(b);
            return vm_push(vm, mjs_value_boolean(result));
        }
        
        case OP_NOT: {
            mjs_value_t a = vm_pop(vm);
            bool result = !mjs_to_boolean(a);
            return vm_push(vm, mjs_value_boolean(result));
        }
        
        // Bitwise operations
        case OP_BIT_AND: {
            mjs_value_t b = vm_pop(vm);
            mjs_value_t a = vm_pop(vm);
            int32_t result = (int32_t)mjs_to_number(a) & (int32_t)mjs_to_number(b);
            return vm_push(vm, mjs_value_number((double)result));
        }
        
        case OP_BIT_OR: {
            mjs_value_t b = vm_pop(vm);
            mjs_value_t a = vm_pop(vm);
            int32_t result = (int32_t)mjs_to_number(a) | (int32_t)mjs_to_number(b);
            return vm_push(vm, mjs_value_number((double)result));
        }
        
        case OP_BIT_XOR: {
            mjs_value_t b = vm_pop(vm);
            mjs_value_t a = vm_pop(vm);
            int32_t result = (int32_t)mjs_to_number(a) ^ (int32_t)mjs_to_number(b);
            return vm_push(vm, mjs_value_number((double)result));
        }
        
        case OP_BIT_NOT: {
            mjs_value_t a = vm_pop(vm);
            int32_t result = ~(int32_t)mjs_to_number(a);
            return vm_push(vm, mjs_value_number((double)result));
        }
        
        case OP_SHL: {
            mjs_value_t b = vm_pop(vm);
            mjs_value_t a = vm_pop(vm);
            int32_t result = (int32_t)mjs_to_number(a) << ((int32_t)mjs_to_number(b) & 0x1F);
            return vm_push(vm, mjs_value_number((double)result));
        }
        
        case OP_SHR: {
            mjs_value_t b = vm_pop(vm);
            mjs_value_t a = vm_pop(vm);
            int32_t result = (int32_t)mjs_to_number(a) >> ((int32_t)mjs_to_number(b) & 0x1F);
            return vm_push(vm, mjs_value_number((double)result));
        }
        
        // Object operations
        case OP_NEW_OBJECT: {
            mjs_object_t* obj = mjs_object_new(vm->context);
            if (!obj) return false;
            return vm_push(vm, mjs_value_object(obj));
        }
        
        case OP_NEW_ARRAY: {
            mjs_array_t* arr = mjs_array_new(vm->context, instr->operand.u32, sizeof(mjs_value_t));
            if (!arr) return false;
            return vm_push(vm, mjs_value_array(arr));
        }
        
        case OP_GET_PROP: {
            if (instr->operand.u32 >= frame->bytecode->string_count) {
                return false;
            }
            const char* prop = frame->bytecode->strings[instr->operand.u32]->data;
            mjs_value_t obj = vm_pop(vm);
            
            if (!mjs_is_object(obj)) {
                return vm_push(vm, mjs_value_undefined());
            }
            
            mjs_value_t result = mjs_object_get_property_value(mjs_get_object(obj), prop);
            return vm_push(vm, result);
        }
        
        case OP_SET_PROP: {
            if (instr->operand.u32 >= frame->bytecode->string_count) {
                return false;
            }
            const char* prop = frame->bytecode->strings[instr->operand.u32]->data;
            mjs_value_t value = vm_pop(vm);
            mjs_value_t obj = vm_pop(vm);
            
            if (!mjs_is_object(obj)) {
                return false;
            }
            
            mjs_object_set_property(mjs_get_object(obj), prop, value);
            break;
        }
        
        case OP_GET_PROP_COMPUTED: {
            mjs_value_t prop = vm_pop(vm);
            mjs_value_t obj = vm_pop(vm);
            
            if (!mjs_is_object(obj)) {
                return vm_push(vm, mjs_value_undefined());
            }
            
            const char* prop_str = mjs_to_string(vm->context, prop);
            if (!prop_str) return false;
            
            mjs_value_t result = mjs_object_get_property_value(mjs_get_object(obj), prop_str);
            return vm_push(vm, result);
        }
        
        case OP_SET_PROP_COMPUTED: {
            mjs_value_t value = vm_pop(vm);
            mjs_value_t prop = vm_pop(vm);
            mjs_value_t obj = vm_pop(vm);
            
            if (!mjs_is_object(obj)) {
                return false;
            }
            
            const char* prop_str = mjs_to_string(vm->context, prop);
            if (!prop_str) return false;
            
            mjs_object_set_property(mjs_get_object(obj), prop_str, value);
            break;
        }
        
        // Array operations
        case OP_ARRAY_PUSH: {
            mjs_value_t value = vm_pop(vm);
            mjs_value_t arr = vm_peek(vm, 0); // Keep array on stack
            
            if (!mjs_is_array(arr)) {
                return false;
            }
            
            return mjs_array_push(mjs_get_array(arr), value);
        }
        
        case OP_ARRAY_POP: {
            mjs_value_t arr = vm_pop(vm);
            
            if (!mjs_is_array(arr)) {
                return vm_push(vm, mjs_value_undefined());
            }
            
            mjs_value_t result = mjs_array_pop(mjs_get_array(arr));
            return vm_push(vm, result);
        }
        
        case OP_ARRAY_GET: {
            mjs_value_t index = vm_pop(vm);
            mjs_value_t arr = vm_pop(vm);
            
            if (!mjs_is_array(arr)) {
                return vm_push(vm, mjs_value_undefined());
            }
            
            size_t idx = (size_t)mjs_to_number(index);
            mjs_value_t result = mjs_array_get(mjs_get_array(arr), idx);
            return vm_push(vm, result);
        }
        
        case OP_ARRAY_SET: {
            mjs_value_t value = vm_pop(vm);
            mjs_value_t index = vm_pop(vm);
            mjs_value_t arr = vm_pop(vm);
            
            if (!mjs_is_array(arr)) {
                return false;
            }
            
            size_t idx = (size_t)mjs_to_number(index);
            return mjs_array_set(mjs_get_array(arr), idx, value);
        }
        
        // Function operations
        case OP_CALL: {
            size_t arg_count = instr->operand.u32;
            
            // Get function and arguments from stack
            mjs_value_t func = vm_peek(vm, arg_count);
            
            if (!mjs_is_function(func)) {
                return false;
            }
            
            // TODO: Implement function calls
            // For now, just pop arguments and function, push undefined
            vm_drop(vm, arg_count + 1);
            return vm_push(vm, mjs_value_undefined());
        }
        
        case OP_RETURN: {
            mjs_value_t return_value = vm_pop(vm);
            
            // Pop current frame
            if (!vm_pop_frame(vm)) {
                return false;
            }
            
            // Push return value
            return vm_push(vm, return_value);
        }
        
        // Control flow
        case OP_JUMP:
            frame->pc = instr->operand.u32;
            break;
            
        case OP_JUMP_IF_TRUE: {
            mjs_value_t condition = vm_pop(vm);
            if (mjs_to_boolean(condition)) {
                frame->pc = instr->operand.u32;
            }
            break;
        }
        
        case OP_JUMP_IF_FALSE: {
            mjs_value_t condition = vm_pop(vm);
            if (!mjs_to_boolean(condition)) {
                frame->pc = instr->operand.u32;
            }
            break;
        }
        
        // Type operations
        case OP_TYPEOF: {
            mjs_value_t value = vm_pop(vm);
            const char* type_str = vm_typeof(value);
            
            mjs_string_t* str = mjs_string_new(vm->context, type_str, strlen(type_str));
            if (!str) return false;
            
            return vm_push(vm, mjs_value_string(str));
        }
        
        default:
            return false; // Unknown opcode
    }
    
    return true;
}

/* Arithmetic operation helpers */
static mjs_value_t vm_add(mjs_vm_t* vm, mjs_value_t a, mjs_value_t b) {
    // Handle string concatenation
    if (mjs_is_string(a) || mjs_is_string(b)) {
        const char* str_a = mjs_to_string(vm->context, a);
        const char* str_b = mjs_to_string(vm->context, b);
        
        if (!str_a || !str_b) {
            return mjs_value_undefined();
        }
        
        // For now, just return undefined for string concatenation
        // TODO: Implement proper string concatenation
        return mjs_value_undefined();
    }
    
    // Numeric addition
    double num_a = mjs_to_number(a);
    double num_b = mjs_to_number(b);
    return mjs_value_number(num_a + num_b);
}

static mjs_value_t vm_subtract(mjs_vm_t* vm, mjs_value_t a, mjs_value_t b) {
    double num_a = mjs_to_number(a);
    double num_b = mjs_to_number(b);
    return mjs_value_number(num_a - num_b);
}

static mjs_value_t vm_multiply(mjs_vm_t* vm, mjs_value_t a, mjs_value_t b) {
    double num_a = mjs_to_number(a);
    double num_b = mjs_to_number(b);
    return mjs_value_number(num_a * num_b);
}

static mjs_value_t vm_divide(mjs_vm_t* vm, mjs_value_t a, mjs_value_t b) {
    double num_a = mjs_to_number(a);
    double num_b = mjs_to_number(b);
    
    if (num_b == 0.0) {
        return mjs_value_number(num_a >= 0 ? INFINITY : -INFINITY);
    }
    
    return mjs_value_number(num_a / num_b);
}

static mjs_value_t vm_modulo(mjs_vm_t* vm, mjs_value_t a, mjs_value_t b) {
    double num_a = mjs_to_number(a);
    double num_b = mjs_to_number(b);
    
    if (num_b == 0.0) {
        return mjs_value_number(NAN);
    }
    
    return mjs_value_number(fmod(num_a, num_b));
}

static mjs_value_t vm_negate(mjs_vm_t* vm, mjs_value_t a) {
    double num = mjs_to_number(a);
    return mjs_value_number(-num);
}

static mjs_value_t vm_unary_plus(mjs_vm_t* vm, mjs_value_t a) {
    double num = mjs_to_number(a);
    return mjs_value_number(num);
}

/* Comparison operation helpers */
static bool vm_equals(mjs_vm_t* vm, mjs_value_t a, mjs_value_t b) {
    // Strict equality for now
    if (mjs_get_type(a) != mjs_get_type(b)) {
        return false;
    }
    
    switch (mjs_get_type(a)) {
        case MJS_TYPE_UNDEFINED:
        case MJS_TYPE_NULL:
            return true;
            
        case MJS_TYPE_BOOLEAN:
            return mjs_get_boolean(a) == mjs_get_boolean(b);
            
        case MJS_TYPE_NUMBER:
            return mjs_get_number(a) == mjs_get_number(b);
            
        case MJS_TYPE_STRING: {
            mjs_string_t* str_a = mjs_get_string(a);
            mjs_string_t* str_b = mjs_get_string(b);
            return mjs_string_compare(str_a, str_b) == 0;
        }
        
        case MJS_TYPE_OBJECT:
        case MJS_TYPE_ARRAY:
        case MJS_TYPE_FUNCTION:
            // Reference equality
            return mjs_get_object(a) == mjs_get_object(b);
            
        default:
            return false;
    }
}

static bool vm_less_than(mjs_vm_t* vm, mjs_value_t a, mjs_value_t b) {
    double num_a = mjs_to_number(a);
    double num_b = mjs_to_number(b);
    return num_a < num_b;
}

static bool vm_less_than_or_equal(mjs_vm_t* vm, mjs_value_t a, mjs_value_t b) {
    double num_a = mjs_to_number(a);
    double num_b = mjs_to_number(b);
    return num_a <= num_b;
}

static bool vm_greater_than(mjs_vm_t* vm, mjs_value_t a, mjs_value_t b) {
    double num_a = mjs_to_number(a);
    double num_b = mjs_to_number(b);
    return num_a > num_b;
}

static bool vm_greater_than_or_equal(mjs_vm_t* vm, mjs_value_t a, mjs_value_t b) {
    double num_a = mjs_to_number(a);
    double num_b = mjs_to_number(b);
    return num_a >= num_b;
}

/* Type operation helpers */
static const char* vm_typeof(mjs_value_t value) {
    switch (mjs_get_type(value)) {
        case MJS_TYPE_UNDEFINED: return "undefined";
        case MJS_TYPE_NULL: return "object"; // JavaScript quirk
        case MJS_TYPE_BOOLEAN: return "boolean";
        case MJS_TYPE_NUMBER: return "number";
        case MJS_TYPE_STRING: return "string";
        case MJS_TYPE_FUNCTION: return "function";
        case MJS_TYPE_OBJECT:
        case MJS_TYPE_ARRAY:
        default: return "object";
    }
}

/* Bytecode management */
mjs_bytecode_t* mjs_bytecode_new(void) {
    mjs_bytecode_t* bytecode = (mjs_bytecode_t*)malloc(sizeof(mjs_bytecode_t));
    if (!bytecode) return NULL;
    
    memset(bytecode, 0, sizeof(mjs_bytecode_t));
    
    bytecode->instruction_capacity = 64;
    bytecode->constant_capacity = 32;
    bytecode->string_capacity = 32;
    
    bytecode->instructions = (mjs_instruction_t*)malloc(sizeof(mjs_instruction_t) * bytecode->instruction_capacity);
    bytecode->constants = (mjs_value_t*)malloc(sizeof(mjs_value_t) * bytecode->constant_capacity);
    bytecode->strings = (mjs_string_t**)malloc(sizeof(mjs_string_t*) * bytecode->string_capacity);
    
    if (!bytecode->instructions || !bytecode->constants || !bytecode->strings) {
        mjs_bytecode_free(bytecode);
        return NULL;
    }
    
    return bytecode;
}

void mjs_bytecode_free(mjs_bytecode_t* bytecode) {
    if (!bytecode) return;
    
    free(bytecode->instructions);
    free(bytecode->constants);
    
    // Free string pool
    if (bytecode->strings) {
        for (size_t i = 0; i < bytecode->string_count; i++) {
            free(bytecode->strings[i]);
        }
        free(bytecode->strings);
    }
    
    free(bytecode);
}

bool mjs_bytecode_emit(mjs_bytecode_t* bytecode, mjs_instruction_t instruction) {
    if (!bytecode) return false;
    
    // Resize if needed
    if (bytecode->instruction_count >= bytecode->instruction_capacity) {
        size_t new_capacity = bytecode->instruction_capacity * 2;
        mjs_instruction_t* new_instructions = (mjs_instruction_t*)realloc(
            bytecode->instructions, sizeof(mjs_instruction_t) * new_capacity);
        
        if (!new_instructions) return false;
        
        bytecode->instructions = new_instructions;
        bytecode->instruction_capacity = new_capacity;
    }
    
    bytecode->instructions[bytecode->instruction_count++] = instruction;
    return true;
}

uint32_t mjs_bytecode_add_constant(mjs_bytecode_t* bytecode, mjs_value_t value) {
    if (!bytecode) return 0;
    
    // Resize if needed
    if (bytecode->constant_count >= bytecode->constant_capacity) {
        size_t new_capacity = bytecode->constant_capacity * 2;
        mjs_value_t* new_constants = (mjs_value_t*)realloc(
            bytecode->constants, sizeof(mjs_value_t) * new_capacity);
        
        if (!new_constants) return 0;
        
        bytecode->constants = new_constants;
        bytecode->constant_capacity = new_capacity;
    }
    
    uint32_t index = (uint32_t)bytecode->constant_count;
    bytecode->constants[bytecode->constant_count++] = value;
    return index;
}

uint32_t mjs_bytecode_add_string(mjs_bytecode_t* bytecode, const char* str) {
    if (!bytecode || !str) return 0;
    
    // Check if string already exists
    for (size_t i = 0; i < bytecode->string_count; i++) {
        if (bytecode->strings[i] && bytecode->strings[i]->data && strcmp(bytecode->strings[i]->data, str) == 0) {
            return (uint32_t)i;
        }
    }
    
    // Resize if needed
    if (bytecode->string_count >= bytecode->string_capacity) {
        size_t new_capacity = bytecode->string_capacity * 2;
        mjs_string_t** new_strings = (mjs_string_t**)realloc(
            bytecode->strings, sizeof(mjs_string_t*) * new_capacity);
        
        if (!new_strings) return 0;
        
        bytecode->strings = new_strings;
        bytecode->string_capacity = new_capacity;
    }
    
    // Add new string - create a simple mjs_string_t
    mjs_string_t* mjs_str = (mjs_string_t*)malloc(sizeof(mjs_string_t));
    if (!mjs_str) return 0;
    
    mjs_str->data = MJS_STRDUP(str);
    mjs_str->length = strlen(str);
    mjs_str->capacity = mjs_str->length + 1;
    mjs_str->is_interned = false;
    mjs_str->next = NULL;
    
    if (!mjs_str->data) {
        free(mjs_str);
        return 0;
    }
    
    uint32_t index = (uint32_t)bytecode->string_count;
    bytecode->strings[bytecode->string_count++] = mjs_str;
    return index;
}

size_t mjs_bytecode_emit_jump(mjs_bytecode_t* bytecode, mjs_opcode_t opcode, uint32_t placeholder) {
    if (!bytecode) return 0;
    
    mjs_instruction_t instruction;
    instruction.opcode = opcode;
    instruction.operand.u32 = placeholder;
    
    size_t jump_index = bytecode->instruction_count;
    mjs_bytecode_emit(bytecode, instruction);
    return jump_index;
}

void mjs_bytecode_patch_jump(mjs_bytecode_t* bytecode, size_t jump_index) {
    if (!bytecode || jump_index >= bytecode->instruction_count) return;
    
    bytecode->instructions[jump_index].operand.u32 = (uint32_t)bytecode->instruction_count;
}

size_t mjs_bytecode_get_current_offset(mjs_bytecode_t* bytecode) {
    return bytecode ? bytecode->instruction_count : 0;
}

/* Debug utilities */
void mjs_vm_dump_stack(mjs_vm_t* vm) {
    if (!vm) return;
    
    printf("Stack (top=%zu):\n", vm->stack_top);
    for (size_t i = 0; i < vm->stack_top; i++) {
        printf("  [%zu]: ", i);
        // Simple value dump without context
        printf("tag=%d", vm->stack[i].tag);
        printf("\n");
    }
}

const char* mjs_opcode_to_string(mjs_opcode_t opcode) {
    switch (opcode) {
        case OP_NOP: return "NOP";
        case OP_LOAD_CONST: return "LOAD_CONST";
        case OP_LOAD_VAR: return "LOAD_VAR";
        case OP_STORE_VAR: return "STORE_VAR";
        case OP_POP: return "POP";
        case OP_DUP: return "DUP";
        case OP_SWAP: return "SWAP";
        case OP_ADD: return "ADD";
        case OP_SUB: return "SUB";
        case OP_MUL: return "MUL";
        case OP_DIV: return "DIV";
        case OP_MOD: return "MOD";
        case OP_NEG: return "NEG";
        case OP_PLUS: return "PLUS";
        case OP_EQ: return "EQ";
        case OP_NE: return "NE";
        case OP_LT: return "LT";
        case OP_LE: return "LE";
        case OP_GT: return "GT";
        case OP_GE: return "GE";
        case OP_AND: return "AND";
        case OP_OR: return "OR";
        case OP_NOT: return "NOT";
        case OP_BIT_AND: return "BIT_AND";
        case OP_BIT_OR: return "BIT_OR";
        case OP_BIT_XOR: return "BIT_XOR";
        case OP_BIT_NOT: return "BIT_NOT";
        case OP_SHL: return "SHL";
        case OP_SHR: return "SHR";
        case OP_NEW_OBJECT: return "NEW_OBJECT";
        case OP_NEW_ARRAY: return "NEW_ARRAY";
        case OP_GET_PROP: return "GET_PROP";
        case OP_SET_PROP: return "SET_PROP";
        case OP_GET_PROP_COMPUTED: return "GET_PROP_COMPUTED";
        case OP_SET_PROP_COMPUTED: return "SET_PROP_COMPUTED";
        case OP_ARRAY_PUSH: return "ARRAY_PUSH";
        case OP_ARRAY_POP: return "ARRAY_POP";
        case OP_ARRAY_GET: return "ARRAY_GET";
        case OP_ARRAY_SET: return "ARRAY_SET";
        case OP_CALL: return "CALL";
        case OP_RETURN: return "RETURN";
        case OP_JUMP: return "JUMP";
        case OP_JUMP_IF_TRUE: return "JUMP_IF_TRUE";
        case OP_JUMP_IF_FALSE: return "JUMP_IF_FALSE";
        case OP_TYPEOF: return "TYPEOF";
        default: return "UNKNOWN";
    }
}

void mjs_bytecode_disassemble(mjs_bytecode_t* bytecode) {
    if (!bytecode) return;
    
    printf("Bytecode disassembly:\n");
    printf("Instructions: %zu\n", bytecode->instruction_count);
    printf("Constants: %zu\n", bytecode->constant_count);
    printf("Strings: %zu\n\n", bytecode->string_count);
    
    for (size_t i = 0; i < bytecode->instruction_count; i++) {
        mjs_instruction_t* instr = &bytecode->instructions[i];
        printf("%04zu: %-20s %u\n", i, 
               mjs_opcode_to_string(instr->opcode), 
               instr->operand.u32);
    }
    
    if (bytecode->constant_count > 0) {
        printf("\nConstants:\n");
        for (size_t i = 0; i < bytecode->constant_count; i++) {
            printf("  [%zu]: ", i);
            // Simple value dump without context
            printf("tag=%d", bytecode->constants[i].tag);
            printf("\n");
        }
    }
    
    if (bytecode->string_count > 0) {
        printf("\nStrings:\n");
        for (size_t i = 0; i < bytecode->string_count; i++) {
            printf("  [%zu]: \"%s\"\n", i, bytecode->strings[i] ? bytecode->strings[i]->data : "(null)");
        }
    }
}