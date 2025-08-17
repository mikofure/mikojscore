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
 * MikoJSCore Object Management
 * JavaScript object creation, property access, and manipulation
 */

#include "mikojs_internal.h"
#include "gc.h"

/* Object creation */
mjs_object_t* mjs_object_new(mjs_context_t* ctx) {
    if (!ctx) return NULL;
    
    mjs_object_t* obj = MJS_GC_ALLOC(ctx->runtime->gc, mjs_object_t, GC_TYPE_OBJECT);
    if (!obj) return NULL;
    
    obj->properties = NULL;
    obj->prototype = NULL;
    obj->extensible = true;
    obj->property_count = 0;
    
    return obj;
}

void mjs_object_free(mjs_object_t* obj) {
    if (!obj) return;
    
    // Free all properties
    mjs_property_t* prop = obj->properties;
    while (prop) {
        mjs_property_t* next = prop->next;
        if (prop->key) {
            mjs_string_free(prop->key);
        }
        MJS_FREE(prop);
        prop = next;
    }
    
    // Note: Don't free the object itself here,
    // as it's managed by the garbage collector
}

/* Property management */
mjs_property_t* mjs_object_get_property(mjs_object_t* obj, const char* key) {
    if (!obj || !key) return NULL;
    
    mjs_property_t* prop = obj->properties;
    while (prop) {
        if (prop->key && MJS_STREQ(prop->key->data, key)) {
            return prop;
        }
        prop = prop->next;
    }
    
    return NULL;
}

mjs_value_t mjs_object_get_property_value(mjs_object_t* obj, const char* key) {
    if (!obj || !key) return mjs_value_undefined();
    
    mjs_property_t* prop = mjs_object_get_property(obj, key);
    if (prop) {
        return prop->value;
    }
    
    return mjs_value_undefined();
}

void mjs_object_set_property(mjs_object_t* obj, const char* key, mjs_value_t value) {
    if (!obj || !key) return;
    
    // Check if property already exists
    mjs_property_t* existing = mjs_object_get_property(obj, key);
    if (existing) {
        if (existing->writable) {
            existing->value = value;
        }
        return;
    }
    
    // Create new property
    mjs_property_t* prop = MJS_MALLOC(sizeof(mjs_property_t));
    if (!prop) return;
    
    // Note: This is a simplified implementation
    // In a real implementation, we'd need to get the context to create the string
    prop->key = NULL; // TODO: Create string from key
    prop->value = value;
    prop->writable = true;
    prop->enumerable = true;
    prop->configurable = true;
    prop->next = obj->properties;
    
    obj->properties = prop;
    obj->property_count++;
}

bool mjs_object_has_property(mjs_object_t* obj, const char* key) {
    return mjs_object_get_property(obj, key) != NULL;
}

bool mjs_object_delete_property(mjs_object_t* obj, const char* key) {
    if (!obj || !key) return false;
    
    mjs_property_t* prev = NULL;
    mjs_property_t* prop = obj->properties;
    
    while (prop) {
        if (prop->key && MJS_STREQ(prop->key->data, key)) {
            if (!prop->configurable) {
                return false; // Cannot delete non-configurable property
            }
            
            if (prev) {
                prev->next = prop->next;
            } else {
                obj->properties = prop->next;
            }
            
            if (prop->key) {
                mjs_string_free(prop->key);
            }
            MJS_FREE(prop);
            obj->property_count--;
            return true;
        }
        prev = prop;
        prop = prop->next;
    }
    
    return true; // Property doesn't exist, deletion "succeeds"
}

/* Property descriptor operations */
mjs_result_t mjs_object_define_property(mjs_context_t* ctx, mjs_object_t* obj, const char* key,
                                       mjs_value_t value, bool writable, bool enumerable, bool configurable) {
    if (!ctx || !obj || !key) return MJS_ERROR_TYPE;
    
    if (!obj->extensible) {
        return MJS_ERROR_TYPE; // Cannot add property to non-extensible object
    }
    
    // Check if property already exists
    mjs_property_t* existing = mjs_object_get_property(obj, key);
    if (existing) {
        if (!existing->configurable) {
            return MJS_ERROR_TYPE; // Cannot redefine non-configurable property
        }
        
        existing->value = value;
        existing->writable = writable;
        existing->enumerable = enumerable;
        existing->configurable = configurable;
        return MJS_OK;
    }
    
    // Create new property
    mjs_property_t* prop = MJS_MALLOC(sizeof(mjs_property_t));
    if (!prop) return MJS_ERROR_MEMORY;
    
    prop->key = mjs_string_new(ctx, key, strlen(key));
    if (!prop->key) {
        MJS_FREE(prop);
        return MJS_ERROR_MEMORY;
    }
    
    prop->value = value;
    prop->writable = writable;
    prop->enumerable = enumerable;
    prop->configurable = configurable;
    prop->next = obj->properties;
    
    obj->properties = prop;
    obj->property_count++;
    
    return MJS_OK;
}

/* Object prototype chain */
void mjs_object_set_prototype(mjs_object_t* obj, mjs_object_t* prototype) {
    if (!obj) return;
    obj->prototype = prototype;
}

mjs_object_t* mjs_object_get_prototype(mjs_object_t* obj) {
    return obj ? obj->prototype : NULL;
}

/* Property enumeration */
char** mjs_object_get_property_names(mjs_context_t* ctx, mjs_object_t* obj, size_t* count) {
    if (!ctx || !obj || !count) return NULL;
    
    *count = 0;
    
    // Count enumerable properties
    mjs_property_t* prop = obj->properties;
    while (prop) {
        if (prop->enumerable) {
            (*count)++;
        }
        prop = prop->next;
    }
    
    if (*count == 0) return NULL;
    
    char** names = MJS_MALLOC(sizeof(char*) * (*count));
    if (!names) {
        *count = 0;
        return NULL;
    }
    
    size_t index = 0;
    prop = obj->properties;
    while (prop && index < *count) {
        if (prop->enumerable && prop->key) {
            names[index] = MJS_STRDUP(prop->key->data);
            index++;
        }
        prop = prop->next;
    }
    
    return names;
}

void mjs_object_free_property_names(char** names, size_t count) {
    if (!names) return;
    
    for (size_t i = 0; i < count; i++) {
        if (names[i]) {
            MJS_FREE(names[i]);
        }
    }
    MJS_FREE(names);
}

/* Object extensibility */
void mjs_object_prevent_extensions(mjs_object_t* obj) {
    if (obj) {
        obj->extensible = false;
    }
}

bool mjs_object_is_extensible(mjs_object_t* obj) {
    return obj ? obj->extensible : false;
}

/* Object sealing and freezing */
void mjs_object_seal(mjs_object_t* obj) {
    if (!obj) return;
    
    obj->extensible = false;
    
    mjs_property_t* prop = obj->properties;
    while (prop) {
        prop->configurable = false;
        prop = prop->next;
    }
}

void mjs_object_freeze(mjs_object_t* obj) {
    if (!obj) return;
    
    obj->extensible = false;
    
    mjs_property_t* prop = obj->properties;
    while (prop) {
        prop->configurable = false;
        prop->writable = false;
        prop = prop->next;
    }
}

bool mjs_object_is_sealed(mjs_object_t* obj) {
    if (!obj || obj->extensible) return false;
    
    mjs_property_t* prop = obj->properties;
    while (prop) {
        if (prop->configurable) {
            return false;
        }
        prop = prop->next;
    }
    
    return true;
}

bool mjs_object_is_frozen(mjs_object_t* obj) {
    if (!obj || obj->extensible) return false;
    
    mjs_property_t* prop = obj->properties;
    while (prop) {
        if (prop->configurable || prop->writable) {
            return false;
        }
        prop = prop->next;
    }
    
    return true;
}

/* Object comparison */
bool mjs_object_equals(mjs_object_t* a, mjs_object_t* b) {
    // Objects are equal only if they are the same object (reference equality)
    return a == b;
}

/* Object cloning (shallow copy) */
mjs_object_t* mjs_object_clone(mjs_context_t* ctx, mjs_object_t* obj) {
    if (!ctx || !obj) return NULL;
    
    mjs_object_t* clone = mjs_object_new(ctx);
    if (!clone) return NULL;
    
    clone->prototype = obj->prototype;
    clone->extensible = obj->extensible;
    
    // Copy all properties
    mjs_property_t* prop = obj->properties;
    while (prop) {
        if (prop->key) {
            mjs_result_t result = mjs_object_define_property(
                ctx, clone, prop->key->data, prop->value,
                prop->writable, prop->enumerable, prop->configurable
            );
            if (result != MJS_OK) {
                mjs_object_free(clone);
                return NULL;
            }
        }
        prop = prop->next;
    }
    
    return clone;
}

/* Object to string conversion */
mjs_string_t* mjs_object_to_string(mjs_context_t* ctx, mjs_object_t* obj) {
    if (!ctx) return NULL;
    
    if (!obj) {
        return mjs_string_new(ctx, "[object Null]", 13);
    }
    
    // TODO: Check for custom toString method
    // For now, return generic object string
    return mjs_string_new(ctx, "[object Object]", 15);
}

/* Object property iteration */
typedef struct {
    mjs_object_t* object;
    mjs_property_t* current;
    bool enumerable_only;
} mjs_property_iterator_t;

mjs_property_iterator_t* mjs_object_create_iterator(mjs_object_t* obj, bool enumerable_only) {
    if (!obj) return NULL;
    
    mjs_property_iterator_t* iter = MJS_MALLOC(sizeof(mjs_property_iterator_t));
    if (!iter) return NULL;
    
    iter->object = obj;
    iter->current = obj->properties;
    iter->enumerable_only = enumerable_only;
    
    return iter;
}

bool mjs_property_iterator_next(mjs_property_iterator_t* iter, const char** key, mjs_value_t* value) {
    if (!iter || !key || !value) return false;
    
    while (iter->current) {
        mjs_property_t* prop = iter->current;
        iter->current = prop->next;
        
        if (!iter->enumerable_only || prop->enumerable) {
            if (prop->key) {
                *key = prop->key->data;
                *value = prop->value;
                return true;
            }
        }
    }
    
    return false;
}

void mjs_property_iterator_free(mjs_property_iterator_t* iter) {
    if (iter) {
        MJS_FREE(iter);
    }
}