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
 * MikoJSCore (MikoJSC) - A lightweight, embeddable JavaScript engine
 */

#ifndef MIKOJS_H
#define MIKOJS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* Version information */
#define MIKOJS_VERSION_MAJOR 0
#define MIKOJS_VERSION_MINOR 1
#define MIKOJS_VERSION_PATCH 0
#define MIKOJS_VERSION_STRING "0.1.0"

/* Forward declarations */
typedef struct mjs_context mjs_context_t;
typedef struct mjs_value mjs_value_t;
typedef struct mjs_runtime mjs_runtime_t;

/* Error codes */
typedef enum {
    MJS_OK = 0,
    MJS_ERROR_SYNTAX = -1,
    MJS_ERROR_RUNTIME = -2,
    MJS_ERROR_MEMORY = -3,
    MJS_ERROR_TYPE = -4,
    MJS_ERROR_REFERENCE = -5,
    MJS_ERROR_RANGE = -6
} mjs_result_t;

/* Value types */
typedef enum {
    MJS_TYPE_UNDEFINED,
    MJS_TYPE_NULL,
    MJS_TYPE_BOOLEAN,
    MJS_TYPE_NUMBER,
    MJS_TYPE_STRING,
    MJS_TYPE_OBJECT,
    MJS_TYPE_FUNCTION,
    MJS_TYPE_ARRAY,
    MJS_TYPE_BIGINT,
    MJS_TYPE_SYMBOL
} mjs_value_type_t;

/* Native function callback */
typedef mjs_value_t (*mjs_native_function_t)(mjs_context_t* ctx, int argc, mjs_value_t* argv);

/* Runtime management */
mjs_runtime_t* mjs_new_runtime(void);
void mjs_free_runtime(mjs_runtime_t* rt);

/* Context management */
mjs_context_t* mjs_new_context(mjs_runtime_t* rt);
void mjs_free_context(mjs_context_t* ctx);

/* Script execution */
mjs_result_t mjs_eval(mjs_context_t* ctx, const char* source, const char* filename, mjs_value_t* result);
mjs_result_t mjs_eval_file(mjs_context_t* ctx, const char* filename, mjs_value_t* result);

/* Value creation and manipulation */
mjs_value_t mjs_undefined(void);
mjs_value_t mjs_null(void);
mjs_value_t mjs_boolean(bool value);
mjs_value_t mjs_number(double value);
mjs_value_t mjs_string(mjs_context_t* ctx, const char* str);
mjs_value_t mjs_object(mjs_context_t* ctx);
mjs_value_t mjs_array(mjs_context_t* ctx);

/* Value type checking */
mjs_value_type_t mjs_get_type(mjs_value_t value);
bool mjs_is_undefined(mjs_value_t value);
bool mjs_is_null(mjs_value_t value);
bool mjs_is_boolean(mjs_value_t value);
bool mjs_is_number(mjs_value_t value);
bool mjs_is_string(mjs_value_t value);
bool mjs_is_object(mjs_value_t value);
bool mjs_is_function(mjs_value_t value);
bool mjs_is_array(mjs_value_t value);

/* Value conversion */
bool mjs_to_boolean(mjs_value_t value);
double mjs_to_number(mjs_value_t value);
const char* mjs_to_string(mjs_context_t* ctx, mjs_value_t value);

/* Object property access */
mjs_result_t mjs_get_property(mjs_context_t* ctx, mjs_value_t object, const char* key, mjs_value_t* result);
mjs_result_t mjs_set_property(mjs_context_t* ctx, mjs_value_t object, const char* key, mjs_value_t value);
mjs_result_t mjs_has_property(mjs_context_t* ctx, mjs_value_t object, const char* key, bool* result);
mjs_result_t mjs_delete_property(mjs_context_t* ctx, mjs_value_t object, const char* key);

/* Array operations */
mjs_result_t mjs_get_array_length(mjs_context_t* ctx, mjs_value_t array, uint32_t* length);
mjs_result_t mjs_get_array_element(mjs_context_t* ctx, mjs_value_t array, uint32_t index, mjs_value_t* result);
mjs_result_t mjs_set_array_element(mjs_context_t* ctx, mjs_value_t array, uint32_t index, mjs_value_t value);

/* Function calls */
mjs_result_t mjs_call_function(mjs_context_t* ctx, mjs_value_t function, mjs_value_t this_arg, int argc, mjs_value_t* argv, mjs_value_t* result);

/* Native function binding */
mjs_result_t mjs_define_function(mjs_context_t* ctx, mjs_value_t object, const char* name, mjs_native_function_t func);
mjs_result_t mjs_define_global_function(mjs_context_t* ctx, const char* name, mjs_native_function_t func);

/* Global object access */
mjs_value_t mjs_get_global_object(mjs_context_t* ctx);

/* Error handling */
const char* mjs_get_error_message(mjs_context_t* ctx);
void mjs_clear_error(mjs_context_t* ctx);

/* Memory management */
void mjs_gc(mjs_context_t* ctx);
size_t mjs_get_memory_usage(mjs_context_t* ctx);

/* Utility functions */
const char* mjs_get_version(void);
void mjs_dump_value(mjs_context_t* ctx, mjs_value_t value);

#ifdef __cplusplus
}
#endif

#endif /* MIKOJS_H */