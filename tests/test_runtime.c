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
 * MikoJSCore Runtime Tests
 */

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

static int test_runtime_creation(void) {
    TEST_SUITE_BEGIN("Runtime Creation and Destruction");
    
    mjs_runtime_t* runtime = mjs_new_runtime();
    TEST_ASSERT(runtime != NULL, "Runtime creation");
    
    mjs_context_t* ctx = mjs_new_context(runtime);
    TEST_ASSERT(ctx != NULL, "Context creation");
    
    mjs_free_context(ctx);
    mjs_free_runtime(runtime);
    
    return 0;
}

static int test_value_creation(void) {
    TEST_SUITE_BEGIN("Value Creation and Type Checking");
    
    mjs_runtime_t* runtime = mjs_new_runtime();
    mjs_context_t* ctx = mjs_new_context(runtime);
    
    // Test undefined
    mjs_value_t undef = mjs_value_undefined();
    TEST_ASSERT(mjs_is_undefined(undef), "Undefined value creation");
    TEST_ASSERT(mjs_get_type(undef) == MJS_TYPE_UNDEFINED, "Undefined type check");
    
    // Test null
    mjs_value_t null_val = mjs_value_null();
    TEST_ASSERT(mjs_is_null(null_val), "Null value creation");
    TEST_ASSERT(mjs_get_type(null_val) == MJS_TYPE_NULL, "Null type check");
    
    // Test boolean
    mjs_value_t bool_val = mjs_value_boolean(true);
    TEST_ASSERT(mjs_is_boolean(bool_val), "Boolean value creation");
    TEST_ASSERT(mjs_get_boolean(bool_val) == true, "Boolean value retrieval");
    
    // Test number
    mjs_value_t num_val = mjs_value_number(42.5);
    TEST_ASSERT(mjs_is_number(num_val), "Number value creation");
    TEST_ASSERT(mjs_get_number(num_val) == 42.5, "Number value retrieval");
    
    // Test string
    mjs_string_t* str = mjs_string_new(ctx, "Hello, World!", 13);
    TEST_ASSERT(str != NULL, "String creation");
    
    mjs_value_t str_val = mjs_value_string(str);
    TEST_ASSERT(mjs_is_string(str_val), "String value creation");
    
    mjs_free_context(ctx);
    mjs_free_runtime(runtime);
    
    return 0;
}

static int test_type_conversion(void) {
    TEST_SUITE_BEGIN("Type Conversion");
    
    mjs_runtime_t* runtime = mjs_new_runtime();
    mjs_context_t* ctx = mjs_new_context(runtime);
    
    // Test to boolean conversion
    TEST_ASSERT(mjs_to_boolean(mjs_value_boolean(true)) == true, "Boolean to boolean (true)");
    TEST_ASSERT(mjs_to_boolean(mjs_value_boolean(false)) == false, "Boolean to boolean (false)");
    TEST_ASSERT(mjs_to_boolean(mjs_value_number(0)) == false, "Number 0 to boolean");
    TEST_ASSERT(mjs_to_boolean(mjs_value_number(42)) == true, "Number 42 to boolean");
    TEST_ASSERT(mjs_to_boolean(mjs_value_undefined()) == false, "Undefined to boolean");
    TEST_ASSERT(mjs_to_boolean(mjs_value_null()) == false, "Null to boolean");
    
    // Test to number conversion
    TEST_ASSERT(mjs_to_number(mjs_value_number(42.5)) == 42.5, "Number to number");
    TEST_ASSERT(mjs_to_number(mjs_value_boolean(true)) == 1.0, "Boolean true to number");
    TEST_ASSERT(mjs_to_number(mjs_value_boolean(false)) == 0.0, "Boolean false to number");
    
    mjs_free_context(ctx);
    mjs_free_runtime(runtime);
    
    return 0;
}

static int test_object_operations(void) {
    TEST_SUITE_BEGIN("Object Operations");
    
    mjs_runtime_t* runtime = mjs_new_runtime();
    mjs_context_t* ctx = mjs_new_context(runtime);
    
    // Test object creation
    mjs_object_t* obj = mjs_object_new(ctx);
    TEST_ASSERT(obj != NULL, "Object creation");
    
    mjs_value_t obj_val = mjs_value_object(obj);
    TEST_ASSERT(mjs_is_object(obj_val), "Object value creation");
    
    // Test property operations
    mjs_value_t prop_val = mjs_value_number(123);
    bool set_result = mjs_object_set_property(obj, "testProp", prop_val);
    TEST_ASSERT(set_result, "Property setting");
    
    mjs_value_t retrieved_val;
    bool get_result = mjs_object_get_property(obj, "testProp", &retrieved_val);
    TEST_ASSERT(get_result, "Property retrieval");
    TEST_ASSERT(mjs_get_number(retrieved_val) == 123, "Property value correctness");
    
    mjs_free_context(ctx);
    mjs_free_runtime(runtime);
    
    return 0;
}

static int test_array_operations(void) {
    TEST_SUITE_BEGIN("Array Operations");
    
    mjs_runtime_t* runtime = mjs_new_runtime();
    mjs_context_t* ctx = mjs_new_context(runtime);
    
    // Test array creation
    mjs_array_t* arr = mjs_array_new(ctx);
    TEST_ASSERT(arr != NULL, "Array creation");
    
    mjs_value_t arr_val = mjs_value_array(arr);
    TEST_ASSERT(mjs_is_array(arr_val), "Array value creation");
    
    // Test array operations
    TEST_ASSERT(mjs_array_length(arr) == 0, "Initial array length");
    
    mjs_value_t elem = mjs_value_number(42);
    bool push_result = mjs_array_push(arr, elem);
    TEST_ASSERT(push_result, "Array push operation");
    TEST_ASSERT(mjs_array_length(arr) == 1, "Array length after push");
    
    mjs_value_t retrieved = mjs_array_get(arr, 0);
    TEST_ASSERT(mjs_get_number(retrieved) == 42, "Array element retrieval");
    
    mjs_free_context(ctx);
    mjs_free_runtime(runtime);
    
    return 0;
}

int test_runtime_run(void) {
    printf("\n=== Running Runtime Tests ===\n");
    
    int result = 0;
    
    result |= test_runtime_creation();
    result |= test_value_creation();
    result |= test_type_conversion();
    result |= test_object_operations();
    result |= test_array_operations();
    
    if (result == 0) {
        printf("\n✅ All runtime tests passed!\n");
    } else {
        printf("\n❌ Some runtime tests failed.\n");
    }
    
    return result;
}