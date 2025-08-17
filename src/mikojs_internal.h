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
 * MikoJSCore Internal Definitions
 * This file contains internal structures and definitions used throughout
 * the MikoJS engine implementation.
 */

#ifndef MIKOJS_INTERNAL_H
#define MIKOJS_INTERNAL_H

#define _POSIX_C_SOURCE 200809L
#include "mikojs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Debug macros */
#ifdef DEBUG
#define MJS_DEBUG(fmt, ...) fprintf(stderr, "[MJS DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
#define MJS_DEBUG(fmt, ...)
#endif

#define MJS_ASSERT(cond) assert(cond)
#define MJS_UNREACHABLE() assert(0 && "unreachable")

/* VM state constants */
#define VM_STATE_READY 0
#define VM_STATE_RUNNING 1
#define VM_STATE_ERROR 2

/* Error constants */
#define MJS_ERROR -1

/* GC type constants */
#define GC_TYPE_STRING 1
#define GC_TYPE_ARRAY 2
#define GC_TYPE_OBJECT 3
#define GC_TYPE_FUNCTION 4

/* Literal type constants */
#define LITERAL_NUMBER 0
#define LITERAL_STRING 1
#define LITERAL_BOOLEAN 2
#define LITERAL_NULL 3
#define LITERAL_UNDEFINED 4

/* Memory allocation macros */
#define MJS_MALLOC(size) malloc(size)
#define MJS_CALLOC(count, size) calloc(count, size)
#define MJS_REALLOC(ptr, size) realloc(ptr, size)
#define MJS_FREE(ptr) free(ptr)

/* String utilities */
#define MJS_STREQ(a, b) (strcmp(a, b) == 0)
#define MJS_STRDUP(s) strdup(s)

/* Forward declarations */
typedef struct mjs_string mjs_string_t;
typedef struct mjs_object mjs_object_t;
typedef struct mjs_function mjs_function_t;
typedef struct mjs_array mjs_array_t;
typedef struct mjs_gc mjs_gc_t;
typedef struct mjs_lexer mjs_lexer_t;
typedef struct mjs_parser mjs_parser_t;
typedef struct mjs_vm mjs_vm_t;
typedef struct mjs_bytecode mjs_bytecode_t;

/* GC object type enumeration */
typedef enum {
    MJS_GC_TYPE_STRING,
    MJS_GC_TYPE_OBJECT,
    MJS_GC_TYPE_ARRAY,
    MJS_GC_TYPE_FUNCTION,
    MJS_GC_TYPE_CONTEXT,
    MJS_GC_TYPE_BYTECODE
} mjs_gc_object_type_t;

/* Value representation */
typedef enum {
    MJS_TAG_UNDEFINED = 0,
    MJS_TAG_NULL,
    MJS_TAG_BOOLEAN,
    MJS_TAG_NUMBER,
    MJS_TAG_STRING,
    MJS_TAG_OBJECT,
    MJS_TAG_FUNCTION,
    MJS_TAG_ARRAY,
    MJS_TAG_BIGINT,
    MJS_TAG_SYMBOL
} mjs_value_tag_t;

/* Value structure */
struct mjs_value {
    mjs_value_tag_t tag;
    union {
        bool boolean;
        double number;
        mjs_string_t* string;
        mjs_object_t* object;
        mjs_function_t* function;
        mjs_array_t* array;
        void* ptr;
    } u;
};

/* String object */
struct mjs_string {
    char* data;
    size_t length;
    size_t capacity;
    bool is_interned;
    struct mjs_string* next; /* for string interning */
};

/* Property structure */
typedef struct mjs_property {
    mjs_string_t* key;
    mjs_value_t value;
    bool writable;
    bool enumerable;
    bool configurable;
    struct mjs_property* next;
} mjs_property_t;

/* Object structure */
struct mjs_object {
    mjs_property_t* properties;
    struct mjs_object* prototype;
    bool extensible;
    size_t property_count;
};

/* Function structure */
struct mjs_function {
    enum {
        MJS_FUNCTION_NATIVE,
        MJS_FUNCTION_BYTECODE
    } type;
    union {
        mjs_native_function_t native;
        struct {
            mjs_bytecode_t* bytecode;
            size_t param_count;
            char** param_names;
        } bytecode;
    } u;
    mjs_string_t* name;
    mjs_object_t* scope;
};

/* Array structure */
struct mjs_array {
    mjs_value_t* elements;
    size_t length;
    size_t capacity;
};

/* Runtime structure */
struct mjs_runtime {
    mjs_gc_t* gc;
    mjs_string_t* string_table; /* interned strings */
    size_t memory_limit;
    size_t memory_usage;
};

/* Context structure */
struct mjs_context {
    mjs_runtime_t* runtime;
    mjs_vm_t* vm;
    mjs_value_t global_object;
    mjs_value_t error_value;
    char* error_message;
    bool has_error;
};

/* Internal function declarations */

/* String management */
mjs_string_t* mjs_string_new(mjs_context_t* ctx, const char* data, size_t length);
mjs_string_t* mjs_string_intern(mjs_context_t* ctx, const char* data, size_t length);
void mjs_string_free(mjs_string_t* str);
int mjs_string_compare(const mjs_string_t* a, const mjs_string_t* b);
mjs_string_t* mjs_string_concat(mjs_context_t* ctx, const mjs_string_t* a, const mjs_string_t* b);

/* Object management */
mjs_object_t* mjs_object_new(mjs_context_t* ctx);
void mjs_object_free(mjs_object_t* obj);
mjs_property_t* mjs_object_get_property(mjs_object_t* obj, const char* key);
mjs_value_t mjs_object_get_property_value(mjs_object_t* obj, const char* key);
void mjs_object_set_property(mjs_object_t* obj, const char* key, mjs_value_t value);
mjs_object_t* mjs_get_object(mjs_value_t value);

/* Array management */
mjs_array_t* mjs_array_new(mjs_context_t* ctx, size_t initial_capacity, size_t element_size);
void mjs_array_free(mjs_array_t* arr);
void mjs_array_resize(mjs_array_t* arr, size_t new_size);
size_t mjs_array_length(mjs_array_t* arr);
mjs_value_t mjs_array_get(mjs_array_t* arr, size_t index);
bool mjs_array_set(mjs_array_t* arr, size_t index, mjs_value_t value);
bool mjs_array_push(mjs_array_t* arr, mjs_value_t value);
mjs_value_t mjs_array_pop(mjs_array_t* arr);
mjs_array_t* mjs_get_array(mjs_value_t value);

/* Function management */
mjs_function_t* mjs_function_new_native(mjs_context_t* ctx, mjs_native_function_t func, const char* name);
mjs_function_t* mjs_function_new_bytecode(mjs_context_t* ctx, mjs_bytecode_t* bytecode, const char* name);
void mjs_function_free(mjs_function_t* func);

/* Value utilities */
void mjs_value_mark(mjs_value_t value);
void mjs_value_free(mjs_value_t value);
bool mjs_value_equals(mjs_value_t a, mjs_value_t b);

/* Type checking and conversion */
bool mjs_is_array(mjs_value_t value);
bool mjs_is_object(mjs_value_t value);
bool mjs_is_boolean(mjs_value_t value);
bool mjs_is_number(mjs_value_t value);
bool mjs_is_string(mjs_value_t value);
bool mjs_to_boolean(mjs_value_t value);
double mjs_to_number(mjs_value_t value);
mjs_value_t mjs_value_undefined(void);
mjs_value_t mjs_value_null(void);
mjs_value_t mjs_value_boolean(bool b);
mjs_value_t mjs_value_number(double n);
mjs_value_t mjs_value_string(mjs_string_t* str);
mjs_value_t mjs_value_array(mjs_array_t* arr);
mjs_value_t mjs_value_object(mjs_object_t* obj);

/* Value accessors */
bool mjs_get_boolean(mjs_value_t value);
double mjs_get_number(mjs_value_t value);
mjs_string_t* mjs_get_string(mjs_value_t value);

/* Context functions */
bool mjs_context_get_variable(mjs_context_t* ctx, const char* name, mjs_value_t* value);
bool mjs_context_set_variable(mjs_context_t* ctx, const char* name, mjs_value_t value);

/* VM execution */
mjs_result_t mjs_vm_execute(mjs_vm_t* vm, mjs_bytecode_t* bytecode, mjs_value_t* result);

/* Error handling */
void mjs_set_error(mjs_context_t* ctx, mjs_result_t code, const char* message);
void mjs_set_error_fmt(mjs_context_t* ctx, mjs_result_t code, const char* fmt, ...);

#endif /* MIKOJS_INTERNAL_H */