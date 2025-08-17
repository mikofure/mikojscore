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
 * MikoJSCore Runtime Implementation
 * Core runtime and context management
 */

#include "mikojs_internal.h"
#include "gc.h"
#include "vm.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef NAN
#define NAN (0.0/0.0)
#endif

#ifndef INFINITY
#define INFINITY (1.0/0.0)
#endif

/* Runtime management */
mjs_runtime_t* mjs_new_runtime(void) {
    mjs_runtime_t* rt = MJS_MALLOC(sizeof(mjs_runtime_t));
    if (!rt) return NULL;
    
    rt->gc = mjs_gc_new(rt);
    if (!rt->gc) {
        MJS_FREE(rt);
        return NULL;
    }
    
    rt->string_table = NULL;
    rt->memory_limit = 64 * 1024 * 1024; // 64MB default
    rt->memory_usage = 0;
    
    return rt;
}

void mjs_free_runtime(mjs_runtime_t* rt) {
    if (!rt) return;
    
    // Free string table
    mjs_string_t* str = rt->string_table;
    while (str) {
        mjs_string_t* next = str->next;
        mjs_string_free(str);
        str = next;
    }
    
    if (rt->gc) {
        mjs_gc_free(rt->gc);
    }
    
    MJS_FREE(rt);
}

/* Context management */
mjs_context_t* mjs_new_context(mjs_runtime_t* rt) {
    if (!rt) return NULL;
    
    mjs_context_t* ctx = MJS_MALLOC(sizeof(mjs_context_t));
    if (!ctx) return NULL;
    
    ctx->runtime = rt;
    ctx->vm = mjs_vm_new(ctx);
    if (!ctx->vm) {
        MJS_FREE(ctx);
        return NULL;
    }
    
    // Create global object
    ctx->global_object = mjs_object(ctx);
    
    // Initialize built-in objects and functions
    // TODO: Add built-in objects like Object, Array, Function, etc.
    
    ctx->error_value = mjs_undefined();
    ctx->error_message = NULL;
    ctx->has_error = false;
    
    return ctx;
}

void mjs_free_context(mjs_context_t* ctx) {
    if (!ctx) return;
    
    if (ctx->vm) {
        mjs_vm_free(ctx->vm);
    }
    
    if (ctx->error_message) {
        MJS_FREE(ctx->error_message);
    }
    
    MJS_FREE(ctx);
}

/* Value creation */
mjs_value_t mjs_undefined(void) {
    mjs_value_t value;
    value.tag = MJS_TAG_UNDEFINED;
    return value;
}

mjs_value_t mjs_null(void) {
    mjs_value_t value;
    value.tag = MJS_TAG_NULL;
    return value;
}

mjs_value_t mjs_boolean(bool val) {
    mjs_value_t value;
    value.tag = MJS_TAG_BOOLEAN;
    value.u.boolean = val;
    return value;
}

mjs_value_t mjs_number(double val) {
    mjs_value_t value;
    value.tag = MJS_TAG_NUMBER;
    value.u.number = val;
    return value;
}

mjs_value_t mjs_string(mjs_context_t* ctx, const char* str) {
    if (!ctx || !str) return mjs_undefined();
    
    mjs_string_t* string_obj = mjs_string_new(ctx, str, strlen(str));
    if (!string_obj) return mjs_undefined();
    
    mjs_value_t value;
    value.tag = MJS_TAG_STRING;
    value.u.string = string_obj;
    return value;
}

mjs_value_t mjs_object(mjs_context_t* ctx) {
    if (!ctx) return mjs_undefined();
    
    mjs_object_t* obj = mjs_object_new(ctx);
    if (!obj) return mjs_undefined();
    
    mjs_value_t value;
    value.tag = MJS_TAG_OBJECT;
    value.u.object = obj;
    return value;
}

mjs_value_t mjs_array(mjs_context_t* ctx) {
    if (!ctx) return mjs_undefined();
    
    mjs_array_t* arr = mjs_array_new(ctx, 0, sizeof(mjs_value_t));
    if (!arr) return mjs_undefined();
    
    mjs_value_t value;
    value.tag = MJS_TAG_ARRAY;
    value.u.array = arr;
    return value;
}

/* Value type checking */
mjs_value_type_t mjs_get_type(mjs_value_t value) {
    switch (value.tag) {
        case MJS_TAG_UNDEFINED: return MJS_TYPE_UNDEFINED;
        case MJS_TAG_NULL: return MJS_TYPE_NULL;
        case MJS_TAG_BOOLEAN: return MJS_TYPE_BOOLEAN;
        case MJS_TAG_NUMBER: return MJS_TYPE_NUMBER;
        case MJS_TAG_STRING: return MJS_TYPE_STRING;
        case MJS_TAG_OBJECT: return MJS_TYPE_OBJECT;
        case MJS_TAG_FUNCTION: return MJS_TYPE_FUNCTION;
        case MJS_TAG_ARRAY: return MJS_TYPE_ARRAY;
        case MJS_TAG_BIGINT: return MJS_TYPE_BIGINT;
        case MJS_TAG_SYMBOL: return MJS_TYPE_SYMBOL;
        default: return MJS_TYPE_UNDEFINED;
    }
}

bool mjs_is_undefined(mjs_value_t value) {
    return value.tag == MJS_TAG_UNDEFINED;
}

bool mjs_is_null(mjs_value_t value) {
    return value.tag == MJS_TAG_NULL;
}

bool mjs_is_boolean(mjs_value_t value) {
    return value.tag == MJS_TAG_BOOLEAN;
}

bool mjs_is_number(mjs_value_t value) {
    return value.tag == MJS_TAG_NUMBER;
}

bool mjs_is_string(mjs_value_t value) {
    return value.tag == MJS_TAG_STRING;
}

bool mjs_is_object(mjs_value_t value) {
    return value.tag == MJS_TAG_OBJECT;
}

bool mjs_is_function(mjs_value_t value) {
    return value.tag == MJS_TAG_FUNCTION;
}

bool mjs_is_array(mjs_value_t value) {
    return value.tag == MJS_TAG_ARRAY;
}

/* Value conversion */
bool mjs_to_boolean(mjs_value_t value) {
    switch (value.tag) {
        case MJS_TAG_UNDEFINED:
        case MJS_TAG_NULL:
            return false;
        case MJS_TAG_BOOLEAN:
            return value.u.boolean;
        case MJS_TAG_NUMBER:
            return value.u.number != 0.0 && !isnan(value.u.number);
        case MJS_TAG_STRING:
            return value.u.string && value.u.string->length > 0;
        case MJS_TAG_OBJECT:
        case MJS_TAG_FUNCTION:
        case MJS_TAG_ARRAY:
            return true;
        default:
            return false;
    }
}

double mjs_to_number(mjs_value_t value) {
    switch (value.tag) {
        case MJS_TAG_UNDEFINED:
            return NAN;
        case MJS_TAG_NULL:
            return 0.0;
        case MJS_TAG_BOOLEAN:
            return value.u.boolean ? 1.0 : 0.0;
        case MJS_TAG_NUMBER:
            return value.u.number;
        case MJS_TAG_STRING:
            if (!value.u.string || value.u.string->length == 0) {
                return 0.0;
            }
            // TODO: Implement proper string to number conversion
            return strtod(value.u.string->data, NULL);
        default:
            return NAN;
    }
}

const char* mjs_to_string(mjs_context_t* ctx, mjs_value_t value) {
    switch (value.tag) {
        case MJS_TAG_UNDEFINED:
            return "undefined";
        case MJS_TAG_NULL:
            return "null";
        case MJS_TAG_BOOLEAN:
            return value.u.boolean ? "true" : "false";
        case MJS_TAG_NUMBER: {
            // TODO: Implement proper number to string conversion
            static char buffer[32];
            snprintf(buffer, sizeof(buffer), "%.15g", value.u.number);
            return buffer;
        }
        case MJS_TAG_STRING:
            return value.u.string ? value.u.string->data : "";
        case MJS_TAG_OBJECT:
            return "[object Object]";
        case MJS_TAG_FUNCTION:
            return "[object Function]";
        case MJS_TAG_ARRAY:
            return "[object Array]";
        default:
            return "";
    }
}

/* Value accessors */
bool mjs_get_boolean(mjs_value_t value) {
    if (value.tag != MJS_TAG_BOOLEAN) return false;
    return value.u.boolean;
}

double mjs_get_number(mjs_value_t value) {
    if (value.tag != MJS_TAG_NUMBER) return 0.0;
    return value.u.number;
}

mjs_string_t* mjs_get_string(mjs_value_t value) {
    if (value.tag != MJS_TAG_STRING) return NULL;
    return value.u.string;
}

mjs_object_t* mjs_get_object(mjs_value_t value) {
    if (value.tag != MJS_TAG_OBJECT) return NULL;
    return value.u.object;
}

mjs_array_t* mjs_get_array(mjs_value_t value) {
    if (value.tag != MJS_TAG_ARRAY) return NULL;
    return value.u.array;
}

/* Internal value creation functions */
mjs_value_t mjs_value_undefined(void) {
    return mjs_undefined();
}

mjs_value_t mjs_value_null(void) {
    return mjs_null();
}

mjs_value_t mjs_value_boolean(bool b) {
    return mjs_boolean(b);
}

mjs_value_t mjs_value_number(double n) {
    return mjs_number(n);
}

mjs_value_t mjs_value_string(mjs_string_t* str) {
    if (!str) return mjs_undefined();
    mjs_value_t value;
    value.tag = MJS_TAG_STRING;
    value.u.string = str;
    return value;
}

mjs_value_t mjs_value_array(mjs_array_t* arr) {
    if (!arr) return mjs_undefined();
    mjs_value_t value;
    value.tag = MJS_TAG_ARRAY;
    value.u.array = arr;
    return value;
}

mjs_value_t mjs_value_object(mjs_object_t* obj) {
    if (!obj) return mjs_undefined();
    mjs_value_t value;
    value.tag = MJS_TAG_OBJECT;
    value.u.object = obj;
    return value;
}

/* Global object access */
mjs_value_t mjs_get_global_object(mjs_context_t* ctx) {
    if (!ctx) return mjs_undefined();
    return ctx->global_object;
}

/* Error handling */
const char* mjs_get_error_message(mjs_context_t* ctx) {
    if (!ctx) return NULL;
    return ctx->error_message;
}

void mjs_clear_error(mjs_context_t* ctx) {
    if (!ctx) return;
    
    ctx->has_error = false;
    ctx->error_value = mjs_undefined();
    
    if (ctx->error_message) {
        MJS_FREE(ctx->error_message);
        ctx->error_message = NULL;
    }
}

void mjs_set_error(mjs_context_t* ctx, mjs_result_t code, const char* message) {
    if (!ctx) return;
    
    ctx->has_error = true;
    
    if (ctx->error_message) {
        MJS_FREE(ctx->error_message);
    }
    
    if (message) {
        ctx->error_message = MJS_STRDUP(message);
    } else {
        ctx->error_message = NULL;
    }
}

void mjs_set_error_fmt(mjs_context_t* ctx, mjs_result_t code, const char* fmt, ...) {
    if (!ctx) return;
    
    va_list args;
    va_start(args, fmt);
    
    if (ctx->error_message) {
        MJS_FREE(ctx->error_message);
    }
    
    ctx->error_message = MJS_MALLOC(256);
    if (ctx->error_message) {
        vsnprintf(ctx->error_message, 256, fmt, args);
    }
    
    ctx->has_error = true;
    
    va_end(args);
}

/* Context variable access */
bool mjs_context_get_variable(mjs_context_t* ctx, const char* name, mjs_value_t* value) {
    if (!ctx || !name || !value) {
        if (value) *value = mjs_undefined();
        return false;
    }
    
    // Simple implementation - look up in global object
    mjs_object_t* global = ctx->global_object.u.object;
    if (!global) {
        *value = mjs_undefined();
        return false;
    }
    
    // Search through properties linked list
    mjs_property_t* prop = global->properties;
    while (prop) {
        if (prop->key && prop->key->data && strcmp(prop->key->data, name) == 0) {
            *value = prop->value;
            return true;
        }
        prop = prop->next;
    }
    
    *value = mjs_undefined();
    return false;
}

bool mjs_context_set_variable(mjs_context_t* ctx, const char* name, mjs_value_t value) {
    if (!ctx || !name) return false;
    
    // Simple implementation - set in global object
    mjs_object_t* global = ctx->global_object.u.object;
    if (!global) return false;
    
    // Look for existing property
    mjs_property_t* prop = global->properties;
    while (prop) {
        if (prop->key && prop->key->data && strcmp(prop->key->data, name) == 0) {
            prop->value = value;
            return true;
        }
        prop = prop->next;
    }
    
    // Create new property
    mjs_property_t* new_prop = (mjs_property_t*)MJS_MALLOC(sizeof(mjs_property_t));
    if (!new_prop) return false;
    
    new_prop->key = mjs_string_new(ctx, name, strlen(name));
    new_prop->value = value;
    new_prop->writable = true;
    new_prop->enumerable = true;
    new_prop->configurable = true;
    new_prop->next = global->properties;
    global->properties = new_prop;
    global->property_count++;
    
    return true;
}

/* Script execution */
mjs_result_t mjs_eval(mjs_context_t* ctx, const char* source, const char* filename, mjs_value_t* result) {
    if (!ctx || !source || !result) {
        if (result) *result = mjs_undefined();
        return MJS_ERROR_TYPE;
    }
    
    // Simple implementation - just return undefined for now
    // In a full implementation, this would:
    // 1. Parse the source code into AST
    // 2. Compile AST to bytecode
    // 3. Execute bytecode in VM
    *result = mjs_undefined();
    return MJS_OK;
}

mjs_result_t mjs_eval_file(mjs_context_t* ctx, const char* filename, mjs_value_t* result) {
    if (!ctx || !filename || !result) {
        if (result) *result = mjs_undefined();
        return MJS_ERROR_TYPE;
    }
    
    // Simple implementation - read file and call mjs_eval
    FILE* file = fopen(filename, "r");
    if (!file) {
        *result = mjs_undefined();
        mjs_set_error(ctx, MJS_ERROR_RUNTIME, "Failed to open file");
        return MJS_ERROR_RUNTIME;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (size <= 0) {
        fclose(file);
        *result = mjs_undefined();
        return MJS_OK;
    }
    
    // Read file content
    char* content = (char*)MJS_MALLOC(size + 1);
    if (!content) {
        fclose(file);
        *result = mjs_undefined();
        return MJS_ERROR_MEMORY;
    }
    
    size_t read_size = fread(content, 1, size, file);
    content[read_size] = '\0';
    fclose(file);
    
    // Evaluate the content
    mjs_result_t res = mjs_eval(ctx, content, filename, result);
    MJS_FREE(content);
    
    return res;
}

/* Memory management */
void mjs_gc(mjs_context_t* ctx) {
    if (!ctx || !ctx->runtime || !ctx->runtime->gc) return;
    mjs_gc_collect(ctx->runtime->gc);
}

size_t mjs_get_memory_usage(mjs_context_t* ctx) {
    if (!ctx || !ctx->runtime || !ctx->runtime->gc) return 0;
    return mjs_gc_get_memory_usage(ctx->runtime->gc);
}

/* Utility functions */
const char* mjs_get_version(void) {
    return MIKOJS_VERSION_STRING;
}

void mjs_dump_value(mjs_context_t* ctx, mjs_value_t value) {
    switch (value.tag) {
        case MJS_TAG_UNDEFINED:
            printf("undefined");
            break;
        case MJS_TAG_NULL:
            printf("null");
            break;
        case MJS_TAG_BOOLEAN:
            printf("%s", value.u.boolean ? "true" : "false");
            break;
        case MJS_TAG_NUMBER:
            printf("%.15g", value.u.number);
            break;
        case MJS_TAG_STRING:
            printf("\"%s\"", value.u.string ? value.u.string->data : "");
            break;
        case MJS_TAG_OBJECT:
            printf("[object Object]");
            break;
        case MJS_TAG_FUNCTION:
            printf("[object Function]");
            break;
        case MJS_TAG_ARRAY:
            printf("[object Array]");
            break;
        default:
            printf("[unknown]");
            break;
    }
}