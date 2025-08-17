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
 * MikoJSCore VM Tests
 */

#include "../src/vm.h"
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

static int test_vm_creation(void) {
    TEST_SUITE_BEGIN("VM Creation and Destruction");
    
    mjs_runtime_t* runtime = mjs_new_runtime();
    mjs_context_t* ctx = mjs_new_context(runtime);
    
    mjs_vm_t* vm = mjs_vm_new(ctx);
    TEST_ASSERT(vm != NULL, "VM creation");
    
    mjs_vm_free(vm);
    mjs_free_context(ctx);
    mjs_free_runtime(runtime);
    
    return 0;
}

static int test_bytecode_creation(void) {
    TEST_SUITE_BEGIN("Bytecode Creation and Management");
    
    mjs_bytecode_t* bytecode = mjs_bytecode_new();
    TEST_ASSERT(bytecode != NULL, "Bytecode creation");
    
    // Test instruction emission
    mjs_instruction_t instr;
    instr.opcode = OP_LOAD_CONST;
    instr.operand = 0;
    
    bool emit_result = mjs_bytecode_emit(bytecode, instr);
    TEST_ASSERT(emit_result, "Instruction emission");
    
    // Test constant addition
    mjs_value_t const_val = mjs_value_number(42);
    uint32_t const_index = mjs_bytecode_add_constant(bytecode, const_val);
    TEST_ASSERT(const_index == 0, "Constant addition");
    
    // Test string addition
    uint32_t str_index = mjs_bytecode_add_string(bytecode, "test");
    TEST_ASSERT(str_index == 0, "String addition");
    
    mjs_bytecode_free(bytecode);
    
    return 0;
}

static int test_basic_execution(void) {
    TEST_SUITE_BEGIN("Basic VM Execution");
    
    mjs_runtime_t* runtime = mjs_new_runtime();
    mjs_context_t* ctx = mjs_new_context(runtime);
    mjs_vm_t* vm = mjs_vm_new(ctx);
    
    // Create simple bytecode: LOAD_CONST 0, RETURN
    mjs_bytecode_t* bytecode = mjs_bytecode_new();
    
    // Add constant
    mjs_value_t const_val = mjs_value_number(42);
    uint32_t const_index = mjs_bytecode_add_constant(bytecode, const_val);
    
    // Emit instructions
    mjs_instruction_t load_instr = {OP_LOAD_CONST, const_index};
    mjs_instruction_t return_instr = {OP_RETURN, 0};
    
    mjs_bytecode_emit(bytecode, load_instr);
    mjs_bytecode_emit(bytecode, return_instr);
    
    // Execute
    mjs_value_t exec_result;
    mjs_result_t result = mjs_vm_execute(vm, bytecode, &exec_result);
    TEST_ASSERT(result == MJS_OK, "Basic bytecode execution");
    
    mjs_bytecode_free(bytecode);
    mjs_vm_free(vm);
    mjs_free_context(ctx);
    mjs_free_runtime(runtime);
    
    return 0;
}

static int test_arithmetic_operations(void) {
    TEST_SUITE_BEGIN("Arithmetic Operations");
    
    mjs_runtime_t* runtime = mjs_new_runtime();
    mjs_context_t* ctx = mjs_new_context(runtime);
    mjs_vm_t* vm = mjs_vm_new(ctx);
    
    // Create bytecode for: 2 + 3
    mjs_bytecode_t* bytecode = mjs_bytecode_new();
    
    // Add constants
    uint32_t const1 = mjs_bytecode_add_constant(bytecode, mjs_value_number(2));
    uint32_t const2 = mjs_bytecode_add_constant(bytecode, mjs_value_number(3));
    
    // Emit instructions: LOAD_CONST 0, LOAD_CONST 1, ADD, RETURN
    mjs_bytecode_emit(bytecode, (mjs_instruction_t){OP_LOAD_CONST, const1});
    mjs_bytecode_emit(bytecode, (mjs_instruction_t){OP_LOAD_CONST, const2});
    mjs_bytecode_emit(bytecode, (mjs_instruction_t){OP_ADD, 0});
    mjs_bytecode_emit(bytecode, (mjs_instruction_t){OP_RETURN, 0});
    
    // Execute
    mjs_value_t exec_result;
    mjs_result_t result = mjs_vm_execute(vm, bytecode, &exec_result);
    TEST_ASSERT(result == MJS_OK, "Arithmetic bytecode execution");
    
    mjs_bytecode_free(bytecode);
    mjs_vm_free(vm);
    mjs_free_context(ctx);
    mjs_free_runtime(runtime);
    
    return 0;
}

static int test_comparison_operations(void) {
    TEST_SUITE_BEGIN("Comparison Operations");
    
    mjs_runtime_t* runtime = mjs_new_runtime();
    mjs_context_t* ctx = mjs_new_context(runtime);
    mjs_vm_t* vm = mjs_vm_new(ctx);
    
    // Create bytecode for: 5 > 3
    mjs_bytecode_t* bytecode = mjs_bytecode_new();
    
    // Add constants
    uint32_t const1 = mjs_bytecode_add_constant(bytecode, mjs_value_number(5));
    uint32_t const2 = mjs_bytecode_add_constant(bytecode, mjs_value_number(3));
    
    // Emit instructions: LOAD_CONST 0, LOAD_CONST 1, GT, RETURN
    mjs_bytecode_emit(bytecode, (mjs_instruction_t){OP_LOAD_CONST, const1});
    mjs_bytecode_emit(bytecode, (mjs_instruction_t){OP_LOAD_CONST, const2});
    mjs_bytecode_emit(bytecode, (mjs_instruction_t){OP_GT, 0});
    mjs_bytecode_emit(bytecode, (mjs_instruction_t){OP_RETURN, 0});
    
    // Execute
    mjs_value_t exec_result;
    mjs_result_t result = mjs_vm_execute(vm, bytecode, &exec_result);
    TEST_ASSERT(result == MJS_OK, "Comparison bytecode execution");
    
    mjs_bytecode_free(bytecode);
    mjs_vm_free(vm);
    mjs_free_context(ctx);
    mjs_free_runtime(runtime);
    
    return 0;
}

static int test_control_flow(void) {
    TEST_SUITE_BEGIN("Control Flow Operations");
    
    mjs_runtime_t* runtime = mjs_new_runtime();
    mjs_context_t* ctx = mjs_new_context(runtime);
    mjs_vm_t* vm = mjs_vm_new(ctx);
    
    // Create simple jump bytecode
    mjs_bytecode_t* bytecode = mjs_bytecode_new();
    
    // Add constants
    uint32_t true_const = mjs_bytecode_add_constant(bytecode, mjs_value_boolean(true));
    uint32_t result_const = mjs_bytecode_add_constant(bytecode, mjs_value_number(42));
    
    // Emit instructions: LOAD_CONST true, JUMP_IF_TRUE 4, LOAD_CONST 0, JUMP 5, LOAD_CONST result, RETURN
    mjs_bytecode_emit(bytecode, (mjs_instruction_t){OP_LOAD_CONST, true_const});
    mjs_bytecode_emit(bytecode, (mjs_instruction_t){OP_JUMP_IF_TRUE, 4});
    mjs_bytecode_emit(bytecode, (mjs_instruction_t){OP_LOAD_CONST, 0});
    mjs_bytecode_emit(bytecode, (mjs_instruction_t){OP_JUMP, 5});
    mjs_bytecode_emit(bytecode, (mjs_instruction_t){OP_LOAD_CONST, result_const});
    mjs_bytecode_emit(bytecode, (mjs_instruction_t){OP_RETURN, 0});
    
    // Execute
    mjs_value_t exec_result;
    mjs_result_t result = mjs_vm_execute(vm, bytecode, &exec_result);
    TEST_ASSERT(result == MJS_OK, "Control flow bytecode execution");
    
    mjs_bytecode_free(bytecode);
    mjs_vm_free(vm);
    mjs_free_context(ctx);
    mjs_free_runtime(runtime);
    
    return 0;
}

int test_vm_run(void) {
    printf("\n=== Running VM Tests ===\n");
    
    int result = 0;
    
    result |= test_vm_creation();
    result |= test_bytecode_creation();
    result |= test_basic_execution();
    result |= test_arithmetic_operations();
    result |= test_comparison_operations();
    result |= test_control_flow();
    
    if (result == 0) {
        printf("\n✅ All VM tests passed!\n");
    } else {
        printf("\n❌ Some VM tests failed.\n");
    }
    
    return result;
}