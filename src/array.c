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
 * MikoJSCore Array Management
 * JavaScript array creation, indexing, and manipulation
 */

#include "mikojs_internal.h"
#include "gc.h"

/* Array creation */
mjs_array_t* mjs_array_new(mjs_context_t* ctx, size_t initial_capacity, size_t element_size) {
    if (!ctx) return NULL;
    
    mjs_array_t* arr = MJS_GC_ALLOC(ctx->runtime->gc, mjs_array_t, GC_TYPE_ARRAY);
    if (!arr) return NULL;
    
    arr->length = 0;
    arr->capacity = initial_capacity > 0 ? initial_capacity : 4;
    
    arr->elements = MJS_MALLOC(sizeof(mjs_value_t) * arr->capacity);
    if (!arr->elements) {
        return NULL;
    }
    
    // Initialize all elements to undefined
    for (size_t i = 0; i < arr->capacity; i++) {
        arr->elements[i] = mjs_value_undefined();
    }
    
    return arr;
}

void mjs_array_free(mjs_array_t* arr) {
    if (!arr) return;
    
    if (arr->elements) {
        MJS_FREE(arr->elements);
        arr->elements = NULL;
    }
    
    // Note: Don't free the array itself here,
    // as it's managed by the garbage collector
}

/* Array resizing */
static bool mjs_array_ensure_capacity(mjs_array_t* arr, size_t required_capacity) {
    if (!arr || required_capacity <= arr->capacity) {
        return true;
    }
    
    size_t new_capacity = arr->capacity;
    while (new_capacity < required_capacity) {
        new_capacity *= 2;
    }
    
    mjs_value_t* new_elements = MJS_REALLOC(arr->elements, sizeof(mjs_value_t) * new_capacity);
    if (!new_elements) {
        return false;
    }
    
    // Initialize new elements to undefined
    for (size_t i = arr->capacity; i < new_capacity; i++) {
        new_elements[i] = mjs_value_undefined();
    }
    
    arr->elements = new_elements;
    arr->capacity = new_capacity;
    return true;
}

void mjs_array_resize(mjs_array_t* arr, size_t new_size) {
    if (!arr) return;
    
    if (new_size > arr->capacity) {
        // Need to expand capacity
        if (!mjs_array_ensure_capacity(arr, new_size)) {
            return; // Failed to resize
        }
    }
    
    if (new_size > arr->length) {
        // Fill new elements with undefined
        for (size_t i = arr->length; i < new_size; i++) {
            arr->elements[i] = mjs_value_undefined();
        }
    }
    
    arr->length = new_size;
}

/* Array element access */
mjs_value_t mjs_array_get(mjs_array_t* arr, size_t index) {
    if (!arr || index >= arr->length) {
        return mjs_value_undefined();
    }
    
    return arr->elements[index];
}

bool mjs_array_set(mjs_array_t* arr, size_t index, mjs_value_t value) {
    if (!arr) return false;
    
    // If index is beyond current length, extend the array
    if (index >= arr->length) {
        if (!mjs_array_ensure_capacity(arr, index + 1)) {
            return false;
        }
        
        // Fill gaps with undefined
        for (size_t i = arr->length; i < index; i++) {
            arr->elements[i] = mjs_value_undefined();
        }
        
        arr->length = index + 1;
    }
    
    arr->elements[index] = value;
    return true;
}

/* Array length operations */
size_t mjs_array_length(mjs_array_t* arr) {
    return arr ? arr->length : 0;
}

bool mjs_array_set_length(mjs_array_t* arr, size_t new_length) {
    if (!arr) return false;
    
    if (new_length > arr->length) {
        // Extending array
        if (!mjs_array_ensure_capacity(arr, new_length)) {
            return false;
        }
        
        // Fill new elements with undefined
        for (size_t i = arr->length; i < new_length; i++) {
            arr->elements[i] = mjs_value_undefined();
        }
    } else if (new_length < arr->length) {
        // Truncating array - elements beyond new_length become undefined
        for (size_t i = new_length; i < arr->length; i++) {
            arr->elements[i] = mjs_value_undefined();
        }
    }
    
    arr->length = new_length;
    return true;
}

/* Array manipulation methods */
bool mjs_array_push(mjs_array_t* arr, mjs_value_t value) {
    if (!arr) return false;
    
    if (!mjs_array_ensure_capacity(arr, arr->length + 1)) {
        return false;
    }
    
    arr->elements[arr->length] = value;
    arr->length++;
    return true;
}

mjs_value_t mjs_array_pop(mjs_array_t* arr) {
    if (!arr || arr->length == 0) {
        return mjs_value_undefined();
    }
    
    arr->length--;
    mjs_value_t value = arr->elements[arr->length];
    arr->elements[arr->length] = mjs_value_undefined();
    return value;
}

bool mjs_array_unshift(mjs_array_t* arr, mjs_value_t value) {
    if (!arr) return false;
    
    if (!mjs_array_ensure_capacity(arr, arr->length + 1)) {
        return false;
    }
    
    // Shift all elements to the right
    for (size_t i = arr->length; i > 0; i--) {
        arr->elements[i] = arr->elements[i - 1];
    }
    
    arr->elements[0] = value;
    arr->length++;
    return true;
}

mjs_value_t mjs_array_shift(mjs_array_t* arr) {
    if (!arr || arr->length == 0) {
        return mjs_value_undefined();
    }
    
    mjs_value_t value = arr->elements[0];
    
    // Shift all elements to the left
    for (size_t i = 0; i < arr->length - 1; i++) {
        arr->elements[i] = arr->elements[i + 1];
    }
    
    arr->length--;
    arr->elements[arr->length] = mjs_value_undefined();
    return value;
}

/* Array searching */
long mjs_array_index_of(mjs_array_t* arr, mjs_value_t value, size_t start_index) {
    if (!arr || start_index >= arr->length) {
        return -1;
    }
    
    for (size_t i = start_index; i < arr->length; i++) {
        // TODO: Implement proper value equality comparison
        // For now, just compare tags and basic values
        if (arr->elements[i].tag == value.tag) {
            bool equal = false;
            switch (value.tag) {
                case MJS_TAG_UNDEFINED:
                case MJS_TAG_NULL:
                    equal = true;
                    break;
                case MJS_TAG_BOOLEAN:
                    equal = (arr->elements[i].u.boolean == value.u.boolean);
                    break;
                case MJS_TAG_NUMBER:
                    equal = (arr->elements[i].u.number == value.u.number);
                    break;
                default:
                    equal = (arr->elements[i].u.ptr == value.u.ptr);
                    break;
            }
            if (equal) {
                 return (long)i;
             }
         }
    }
    
    return -1;
}

long mjs_array_last_index_of(mjs_array_t* arr, mjs_value_t value, size_t start_index) {
    if (!arr || arr->length == 0) {
        return -1;
    }
    
    size_t start = (start_index < arr->length) ? start_index : arr->length - 1;
    
    for (size_t i = start + 1; i > 0; i--) {
        size_t index = i - 1;
        // TODO: Implement proper value equality comparison
        if (arr->elements[index].tag == value.tag) {
            bool equal = false;
            switch (value.tag) {
                case MJS_TAG_UNDEFINED:
                case MJS_TAG_NULL:
                    equal = true;
                    break;
                case MJS_TAG_BOOLEAN:
                    equal = (arr->elements[index].u.boolean == value.u.boolean);
                    break;
                case MJS_TAG_NUMBER:
                    equal = (arr->elements[index].u.number == value.u.number);
                    break;
                default:
                    equal = (arr->elements[index].u.ptr == value.u.ptr);
                    break;
            }
            if (equal) {
                 return (long)index;
             }
         }
    }
    
    return -1;
}

bool mjs_array_includes(mjs_array_t* arr, mjs_value_t value, size_t start_index) {
    return mjs_array_index_of(arr, value, start_index) != -1;
}

/* Array slicing and splicing */
mjs_array_t* mjs_array_slice(mjs_context_t* ctx, mjs_array_t* arr, long start, long end) {
    if (!ctx || !arr) return NULL;
    
    // Handle negative indices
    if (start < 0) {
        start = (long)arr->length + start;
        if (start < 0) start = 0;
    }
    if (end < 0) {
        end = (long)arr->length + end;
    }
    
    // Clamp to array bounds
    if (start >= (long)arr->length) start = arr->length;
    if (end > (long)arr->length) end = arr->length;
    if (end <= start) {
        return mjs_array_new(ctx, 0, sizeof(mjs_value_t));
    }
    
    size_t slice_length = end - start;
    mjs_array_t* slice = mjs_array_new(ctx, slice_length, sizeof(mjs_value_t));
    if (!slice) return NULL;
    
    for (size_t i = 0; i < slice_length; i++) {
        slice->elements[i] = arr->elements[start + i];
    }
    slice->length = slice_length;
    
    return slice;
}

mjs_array_t* mjs_array_splice(mjs_context_t* ctx, mjs_array_t* arr, size_t start, size_t delete_count, mjs_value_t* items, size_t item_count) {
    if (!ctx || !arr) return NULL;
    
    // Clamp start to array bounds
    if (start > arr->length) start = arr->length;
    
    // Clamp delete_count
    if (delete_count > arr->length - start) {
        delete_count = arr->length - start;
    }
    
    // Create array for deleted elements
    mjs_array_t* deleted = mjs_array_new(ctx, delete_count, sizeof(mjs_value_t));
    if (!deleted) return NULL;
    
    // Copy deleted elements
    for (size_t i = 0; i < delete_count; i++) {
        deleted->elements[i] = arr->elements[start + i];
    }
    deleted->length = delete_count;
    
    // Calculate new array length
    size_t new_length = arr->length - delete_count + item_count;
    
    if (new_length > arr->length) {
        // Array will grow
        if (!mjs_array_ensure_capacity(arr, new_length)) {
            return deleted;
        }
    }
    
    // Move elements after the splice point
    if (item_count != delete_count) {
        size_t move_count = arr->length - start - delete_count;
        if (item_count > delete_count) {
            // Moving right
            for (size_t i = move_count; i > 0; i--) {
                arr->elements[start + item_count + i - 1] = arr->elements[start + delete_count + i - 1];
            }
        } else {
            // Moving left
            for (size_t i = 0; i < move_count; i++) {
                arr->elements[start + item_count + i] = arr->elements[start + delete_count + i];
            }
        }
    }
    
    // Insert new items
    if (items && item_count > 0) {
        for (size_t i = 0; i < item_count; i++) {
            arr->elements[start + i] = items[i];
        }
    }
    
    // Update array length
    arr->length = new_length;
    
    // Clear any remaining elements if array shrunk
    for (size_t i = arr->length; i < arr->capacity; i++) {
        arr->elements[i] = mjs_value_undefined();
    }
    
    return deleted;
}

/* Array concatenation */
mjs_array_t* mjs_array_concat(mjs_context_t* ctx, mjs_array_t* arr1, mjs_array_t* arr2) {
    if (!ctx) return NULL;
    
    size_t len1 = arr1 ? arr1->length : 0;
    size_t len2 = arr2 ? arr2->length : 0;
    size_t total_length = len1 + len2;
    
    mjs_array_t* result = mjs_array_new(ctx, total_length, sizeof(mjs_value_t));
    if (!result) return NULL;
    
    // Copy elements from first array
    if (arr1) {
        for (size_t i = 0; i < len1; i++) {
            result->elements[i] = arr1->elements[i];
        }
    }
    
    // Copy elements from second array
    if (arr2) {
        for (size_t i = 0; i < len2; i++) {
            result->elements[len1 + i] = arr2->elements[i];
        }
    }
    
    result->length = total_length;
    return result;
}

/* Array reversal */
void mjs_array_reverse(mjs_array_t* arr) {
    if (!arr || arr->length <= 1) return;
    
    size_t left = 0;
    size_t right = arr->length - 1;
    
    while (left < right) {
        mjs_value_t temp = arr->elements[left];
        arr->elements[left] = arr->elements[right];
        arr->elements[right] = temp;
        left++;
        right--;
    }
}

/* Array to string conversion */
mjs_string_t* mjs_array_join(mjs_context_t* ctx, mjs_array_t* arr, const char* separator) {
    if (!ctx) return NULL;
    
    if (!arr || arr->length == 0) {
        return mjs_string_new(ctx, "", 0);
    }
    
    const char* sep = separator ? separator : ",";
    size_t sep_len = strlen(sep);
    
    // Calculate approximate result length
    size_t estimated_length = arr->length * 10 + (arr->length - 1) * sep_len;
    char* buffer = MJS_MALLOC(estimated_length + 1);
    if (!buffer) return NULL;
    
    size_t pos = 0;
    
    for (size_t i = 0; i < arr->length; i++) {
        if (i > 0) {
            // Add separator
            if (pos + sep_len >= estimated_length) {
                // Reallocate if needed
                estimated_length *= 2;
                char* new_buffer = MJS_REALLOC(buffer, estimated_length + 1);
                if (!new_buffer) {
                    MJS_FREE(buffer);
                    return NULL;
                }
                buffer = new_buffer;
            }
            memcpy(buffer + pos, sep, sep_len);
            pos += sep_len;
        }
        
        // Convert element to string
        // TODO: Implement proper value to string conversion
        // For now, just handle basic cases
        mjs_value_t value = arr->elements[i];
        const char* str_value = "undefined";
        size_t str_len = 9;
        
        if (mjs_is_undefined(value)) {
            str_value = "";
            str_len = 0;
        } else if (mjs_is_null(value)) {
            str_value = "null";
            str_len = 4;
        } else if (mjs_is_boolean(value)) {
            if (mjs_to_boolean(value)) {
                str_value = "true";
                str_len = 4;
            } else {
                str_value = "false";
                str_len = 5;
            }
        }
        
        // Ensure buffer has enough space
        if (pos + str_len >= estimated_length) {
            estimated_length = pos + str_len + 100;
            char* new_buffer = MJS_REALLOC(buffer, estimated_length + 1);
            if (!new_buffer) {
                MJS_FREE(buffer);
                return NULL;
            }
            buffer = new_buffer;
        }
        
        memcpy(buffer + pos, str_value, str_len);
        pos += str_len;
    }
    
    buffer[pos] = '\0';
    
    mjs_string_t* result = mjs_string_new(ctx, buffer, pos);
    MJS_FREE(buffer);
    return result;
}

/* Array comparison */
bool mjs_array_equals(mjs_array_t* a, mjs_array_t* b) {
    // Arrays are equal only if they are the same array (reference equality)
    return a == b;
}

/* Array cloning */
mjs_array_t* mjs_array_clone(mjs_context_t* ctx, mjs_array_t* arr) {
    if (!ctx || !arr) return NULL;
    
    mjs_array_t* clone = mjs_array_new(ctx, arr->capacity, sizeof(mjs_value_t));
    if (!clone) return NULL;
    
    for (size_t i = 0; i < arr->length; i++) {
        clone->elements[i] = arr->elements[i];
    }
    clone->length = arr->length;
    
    return clone;
}

/* Array iteration helpers */
typedef struct {
    mjs_array_t* array;
    size_t index;
} mjs_array_iterator_t;

mjs_array_iterator_t* mjs_array_create_iterator(mjs_array_t* arr) {
    if (!arr) return NULL;
    
    mjs_array_iterator_t* iter = MJS_MALLOC(sizeof(mjs_array_iterator_t));
    if (!iter) return NULL;
    
    iter->array = arr;
    iter->index = 0;
    
    return iter;
}

bool mjs_array_iterator_next(mjs_array_iterator_t* iter, size_t* index, mjs_value_t* value) {
    if (!iter || !index || !value || iter->index >= iter->array->length) {
        return false;
    }
    
    *index = iter->index;
    *value = iter->array->elements[iter->index];
    iter->index++;
    
    return true;
}

void mjs_array_iterator_free(mjs_array_iterator_t* iter) {
    if (iter) {
        MJS_FREE(iter);
    }
}