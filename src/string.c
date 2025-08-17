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
 * MikoJSCore String Management
 * String creation, manipulation, and interning
 */

#include "mikojs_internal.h"
#include "gc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#ifndef NAN
#define NAN (0.0/0.0)
#endif

#ifndef INFINITY
#define INFINITY (1.0/0.0)
#endif

/* String creation */
mjs_string_t* mjs_string_new(mjs_context_t* ctx, const char* data, size_t length) {
    if (!ctx || !data) return NULL;
    
    mjs_string_t* str = MJS_GC_ALLOC(ctx->runtime->gc, mjs_string_t, GC_TYPE_STRING);
    if (!str) return NULL;
    
    str->length = length;
    str->capacity = length + 1;
    str->data = MJS_MALLOC(str->capacity);
    if (!str->data) {
        MJS_GC_FREE(ctx->runtime->gc, str);
        return NULL;
    }
    
    memcpy(str->data, data, length);
    str->data[length] = '\0';
    str->is_interned = false;
    str->next = NULL;
    
    return str;
}

mjs_string_t* mjs_string_intern(mjs_context_t* ctx, const char* data, size_t length) {
    if (!ctx || !data) return NULL;
    
    // Check if string is already interned
    mjs_string_t* current = ctx->runtime->string_table;
    while (current) {
        if (current->length == length && memcmp(current->data, data, length) == 0) {
            return current;
        }
        current = current->next;
    }
    
    // Create new interned string
    mjs_string_t* str = mjs_string_new(ctx, data, length);
    if (!str) return NULL;
    
    str->is_interned = true;
    str->next = ctx->runtime->string_table;
    ctx->runtime->string_table = str;
    
    return str;
}

void mjs_string_free(mjs_string_t* str) {
    if (!str) return;
    
    if (str->data) {
        MJS_FREE(str->data);
    }
    
    // Note: Don't free the string object itself here,
    // as it's managed by the garbage collector
}

int mjs_string_compare(const mjs_string_t* a, const mjs_string_t* b) {
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;
    
    if (a->length != b->length) {
        return (int)(a->length - b->length);
    }
    
    return memcmp(a->data, b->data, a->length);
}

/* String concatenation */
mjs_string_t* mjs_string_concat(mjs_context_t* ctx, const mjs_string_t* a, const mjs_string_t* b) {
    if (!ctx) return NULL;
    if (!a && !b) return mjs_string_new(ctx, "", 0);
    if (!a) return mjs_string_new(ctx, b->data, b->length);
    if (!b) return mjs_string_new(ctx, a->data, a->length);
    
    size_t total_length = a->length + b->length;
    mjs_string_t* result = MJS_GC_ALLOC(ctx->runtime->gc, mjs_string_t, GC_TYPE_STRING);
    if (!result) return NULL;
    
    result->length = total_length;
    result->capacity = total_length + 1;
    result->data = MJS_MALLOC(result->capacity);
    if (!result->data) {
        MJS_GC_FREE(ctx->runtime->gc, result);
        return NULL;
    }
    
    memcpy(result->data, a->data, a->length);
    memcpy(result->data + a->length, b->data, b->length);
    result->data[total_length] = '\0';
    result->is_interned = false;
    result->next = NULL;
    
    return result;
}

/* String substring */
mjs_string_t* mjs_string_substring(mjs_context_t* ctx, const mjs_string_t* str, size_t start, size_t length) {
    if (!ctx || !str || start >= str->length) {
        return mjs_string_new(ctx, "", 0);
    }
    
    if (start + length > str->length) {
        length = str->length - start;
    }
    
    return mjs_string_new(ctx, str->data + start, length);
}

/* String search */
int mjs_string_index_of(const mjs_string_t* str, const mjs_string_t* search, size_t start_pos) {
    if (!str || !search || search->length == 0 || start_pos >= str->length) {
        return -1;
    }
    
    if (search->length > str->length - start_pos) {
        return -1;
    }
    
    for (size_t i = start_pos; i <= str->length - search->length; i++) {
        if (memcmp(str->data + i, search->data, search->length) == 0) {
            return (int)i;
        }
    }
    
    return -1;
}

/* String case conversion */
mjs_string_t* mjs_string_to_lower(mjs_context_t* ctx, const mjs_string_t* str) {
    if (!ctx || !str) return NULL;
    
    mjs_string_t* result = mjs_string_new(ctx, str->data, str->length);
    if (!result) return NULL;
    
    for (size_t i = 0; i < result->length; i++) {
        result->data[i] = tolower(result->data[i]);
    }
    
    return result;
}

mjs_string_t* mjs_string_to_upper(mjs_context_t* ctx, const mjs_string_t* str) {
    if (!ctx || !str) return NULL;
    
    mjs_string_t* result = mjs_string_new(ctx, str->data, str->length);
    if (!result) return NULL;
    
    for (size_t i = 0; i < result->length; i++) {
        result->data[i] = toupper(result->data[i]);
    }
    
    return result;
}

/* String trimming */
mjs_string_t* mjs_string_trim(mjs_context_t* ctx, const mjs_string_t* str) {
    if (!ctx || !str || str->length == 0) {
        return mjs_string_new(ctx, "", 0);
    }
    
    size_t start = 0;
    size_t end = str->length;
    
    // Trim from start
    while (start < str->length && isspace(str->data[start])) {
        start++;
    }
    
    // Trim from end
    while (end > start && isspace(str->data[end - 1])) {
        end--;
    }
    
    if (start == 0 && end == str->length) {
        // No trimming needed, return copy
        return mjs_string_new(ctx, str->data, str->length);
    }
    
    return mjs_string_new(ctx, str->data + start, end - start);
}

/* String splitting */
mjs_array_t* mjs_string_split(mjs_context_t* ctx, const mjs_string_t* str, const mjs_string_t* separator) {
    if (!ctx || !str) return NULL;
    
    mjs_array_t* result = mjs_array_new(ctx, 0, sizeof(mjs_value_t));
    if (!result) return NULL;
    
    if (!separator || separator->length == 0) {
        // Split into individual characters
        mjs_array_resize(result, str->length);
        for (size_t i = 0; i < str->length; i++) {
            mjs_string_t* char_str = mjs_string_new(ctx, str->data + i, 1);
            if (char_str) {
                mjs_value_t char_value;
                char_value.tag = MJS_TAG_STRING;
                char_value.u.string = char_str;
                result->elements[i] = char_value;
            }
        }
        return result;
    }
    
    size_t start = 0;
    size_t pos = 0;
    
    while (pos <= str->length) {
        int found = mjs_string_index_of(str, separator, pos);
        
        if (found == -1) {
            // Add remaining string
            if (start < str->length) {
                mjs_string_t* part = mjs_string_substring(ctx, str, start, str->length - start);
                if (part) {
                    mjs_array_resize(result, result->length + 1);
                    mjs_value_t part_value;
                    part_value.tag = MJS_TAG_STRING;
                    part_value.u.string = part;
                    result->elements[result->length - 1] = part_value;
                }
            }
            break;
        }
        
        // Add substring before separator
        mjs_string_t* part = mjs_string_substring(ctx, str, start, found - start);
        if (part) {
            mjs_array_resize(result, result->length + 1);
            mjs_value_t part_value;
            part_value.tag = MJS_TAG_STRING;
            part_value.u.string = part;
            result->elements[result->length - 1] = part_value;
        }
        
        start = found + separator->length;
        pos = start;
    }
    
    return result;
}

/* String from number */
mjs_string_t* mjs_string_from_number(mjs_context_t* ctx, double number) {
    if (!ctx) return NULL;
    
    char buffer[32];
    
    if (isnan(number)) {
        strcpy(buffer, "NaN");
    } else if (isinf(number)) {
        strcpy(buffer, number > 0 ? "Infinity" : "-Infinity");
    } else {
        snprintf(buffer, sizeof(buffer), "%.15g", number);
    }
    
    return mjs_string_new(ctx, buffer, strlen(buffer));
}

/* String to number conversion */
double mjs_string_to_number(const mjs_string_t* str) {
    if (!str || str->length == 0) {
        return 0.0;
    }
    
    // Handle special cases
    if (MJS_STREQ(str->data, "NaN")) {
        return NAN;
    }
    if (MJS_STREQ(str->data, "Infinity")) {
        return INFINITY;
    }
    if (MJS_STREQ(str->data, "-Infinity")) {
        return -INFINITY;
    }
    
    // Skip leading whitespace
    const char* start = str->data;
    while (isspace(*start)) start++;
    
    // Parse number
    char* end;
    double result = strtod(start, &end);
    
    // Skip trailing whitespace
    while (isspace(*end)) end++;
    
    // If we didn't consume the entire string, it's not a valid number
    if (*end != '\0') {
        return NAN;
    }
    
    return result;
}

/* String hash function for object property keys */
uint32_t mjs_string_hash(const mjs_string_t* str) {
    if (!str || !str->data) return 0;
    
    uint32_t hash = 5381;
    for (size_t i = 0; i < str->length; i++) {
        hash = ((hash << 5) + hash) + (unsigned char)str->data[i];
    }
    return hash;
}

/* String escape/unescape for JSON-like formatting */
mjs_string_t* mjs_string_escape(mjs_context_t* ctx, const mjs_string_t* str) {
    if (!ctx || !str) return NULL;
    
    // Calculate required size
    size_t escaped_size = 0;
    for (size_t i = 0; i < str->length; i++) {
        char c = str->data[i];
        switch (c) {
            case '\"':
            case '\\':
            case '\b':
            case '\f':
            case '\n':
            case '\r':
            case '\t':
                escaped_size += 2;
                break;
            default:
                if (c < 32) {
                    escaped_size += 6; // \uXXXX
                } else {
                    escaped_size += 1;
                }
                break;
        }
    }
    
    mjs_string_t* result = MJS_GC_ALLOC(ctx->runtime->gc, mjs_string_t, GC_TYPE_STRING);
    if (!result) return NULL;
    
    result->length = escaped_size;
    result->capacity = escaped_size + 1;
    result->data = MJS_MALLOC(result->capacity);
    if (!result->data) {
        MJS_GC_FREE(ctx->runtime->gc, result);
        return NULL;
    }
    
    size_t pos = 0;
    for (size_t i = 0; i < str->length; i++) {
        char c = str->data[i];
        switch (c) {
            case '\"':
                result->data[pos++] = '\\';
                result->data[pos++] = '\"';
                break;
            case '\\':
                result->data[pos++] = '\\';
                result->data[pos++] = '\\';
                break;
            case '\b':
                result->data[pos++] = '\\';
                result->data[pos++] = 'b';
                break;
            case '\f':
                result->data[pos++] = '\\';
                result->data[pos++] = 'f';
                break;
            case '\n':
                result->data[pos++] = '\\';
                result->data[pos++] = 'n';
                break;
            case '\r':
                result->data[pos++] = '\\';
                result->data[pos++] = 'r';
                break;
            case '\t':
                result->data[pos++] = '\\';
                result->data[pos++] = 't';
                break;
            default:
                if (c < 32) {
                    pos += snprintf(result->data + pos, 7, "\\u%04x", (unsigned char)c);
                } else {
                    result->data[pos++] = c;
                }
                break;
        }
    }
    
    result->data[pos] = '\0';
    result->is_interned = false;
    result->next = NULL;
    
    return result;
}