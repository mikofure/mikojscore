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
 * MikoJSCore Garbage Collector
 * Generational mark-and-sweep garbage collector with incremental collection
 */

#ifndef MIKOJS_GC_H
#define MIKOJS_GC_H

#include "mikojs_internal.h"
#include <time.h>

/* GC mark colors */
#define GC_MARK_WHITE 0
#define GC_MARK_GRAY  1
#define GC_MARK_BLACK 2

/* Weak reference callback type */
typedef void (*mjs_weak_ref_callback_t)(void* data);

/* GC object types - use the one from mikojs_internal.h */

/* GC state enumeration */
typedef enum {
    GC_STATE_IDLE,
    GC_STATE_MARKING,
    GC_STATE_SWEEPING,
    GC_STATE_COMPACTING
} mjs_gc_state_t;

/* GC configuration */
typedef struct {
    bool incremental;
    bool generational;
    bool compact;
    size_t threshold_factor;
    size_t collection_threshold;
    size_t max_heap_size;
    bool enable_compaction;
} mjs_gc_config_t;

/* GC object header */
typedef struct mjs_gc_object {
    mjs_gc_object_type_t type;
    bool marked;
    int mark;
    int generation;
    bool in_use;
    size_t size;
    struct mjs_gc_object* next;
    struct mjs_gc_object* prev;
} mjs_gc_object_t;

/* Type alias for compatibility */
typedef mjs_gc_object_t mjs_gc_object_header_t;

/* GC generation */
typedef struct {
    mjs_gc_object_t* objects;
    size_t object_count;
    size_t total_size;
    size_t size;
    size_t capacity;
    size_t threshold;
} mjs_gc_generation_t;

/* GC statistics */
typedef struct {
    size_t total_allocations;
    size_t total_deallocations;
    size_t total_collections;
    size_t collections;
    size_t total_bytes_allocated;
    size_t total_bytes_freed;
    size_t bytes_freed;
    size_t bytes_allocated;
    size_t objects_allocated;
    size_t objects_freed;
    size_t heap_size;
    size_t heap_used;
    size_t young_generation_size;
    size_t old_generation_size;
    size_t peak_memory_usage;
    double last_collection_time;
    double total_collection_time;
    size_t collection_time;
} mjs_gc_stats_t;

/* Garbage collector structure */
struct mjs_gc {
    mjs_runtime_t* runtime;
    
    /* Configuration and state */
    mjs_gc_config_t config;
    mjs_gc_state_t state;
    
    /* Heap management */
    void* heap;
    void* heap_ptr;
    size_t heap_used;
    size_t heap_size;
    size_t incremental_step;
    
    /* Generations (young, old) */
    mjs_gc_generation_t young_generation;
    mjs_gc_generation_t old_generation;
    
    /* Root objects */
    void** roots;
    size_t root_count;
    size_t root_capacity;
    
    /* Gray stack for tri-color marking */
    mjs_gc_object_t** gray_stack;
    size_t gray_count;
    size_t gray_capacity;
    
    /* Collection state */
    bool collecting;
    bool incremental_mode;
    size_t incremental_step_size;
    
    /* Thresholds */
    size_t young_threshold;
    size_t old_threshold;
    size_t memory_limit;
    
    /* Statistics */
    mjs_gc_stats_t stats;
    
    /* Weak references */
    struct mjs_weak_ref* weak_refs;
    size_t weak_ref_count;
    size_t weak_ref_capacity;
};

/* Weak reference structure */
typedef struct mjs_weak_ref {
    void* object;
    mjs_weak_ref_callback_t callback;
    void* callback_data;
    void* userdata;
    bool cleared;
    struct mjs_weak_ref* next;
} mjs_weak_ref_t;

/* GC functions */
mjs_gc_t* mjs_gc_new(mjs_runtime_t* runtime);
void mjs_gc_free(mjs_gc_t* gc);

/* Memory allocation */
void* mjs_gc_alloc(mjs_gc_t* gc, size_t size, mjs_gc_object_type_t type);
void* mjs_gc_realloc(mjs_gc_t* gc, void* ptr, size_t old_size, size_t new_size);
void mjs_gc_free_object(mjs_gc_t* gc, void* ptr);

/* Collection triggers */
bool mjs_gc_collect(mjs_gc_t* gc);
bool mjs_gc_collect_young(mjs_gc_t* gc);
bool mjs_gc_collect_full(mjs_gc_t* gc);
bool mjs_gc_collect_incremental(mjs_gc_t* gc, uint64_t time_limit_us);

/* Root management */
bool mjs_gc_add_root(mjs_gc_t* gc, void* obj);
bool mjs_gc_remove_root(mjs_gc_t* gc, void* obj);
void mjs_gc_push_root(mjs_gc_t* gc, mjs_gc_object_t* obj);
void mjs_gc_pop_root(mjs_gc_t* gc);

/* Marking functions */
void mjs_gc_mark_object(mjs_gc_t* gc, mjs_gc_object_t* obj);
void mjs_gc_mark_value(mjs_gc_t* gc, mjs_value_t value);
void mjs_gc_mark_string(mjs_gc_t* gc, mjs_string_t* str);
void mjs_gc_mark_object_properties(mjs_gc_t* gc, mjs_object_t* obj);
void mjs_gc_mark_array_elements(mjs_gc_t* gc, mjs_array_t* arr);
void mjs_gc_mark_function(mjs_gc_t* gc, mjs_function_t* func);

/* Weak reference management */
mjs_weak_ref_t* mjs_gc_create_weak_ref(mjs_gc_t* gc, void* target);
void mjs_weak_ref_free(mjs_weak_ref_t* weak_ref);
void* mjs_weak_ref_get(mjs_weak_ref_t* weak_ref);
void mjs_gc_clear_weak_ref(mjs_gc_t* gc, mjs_weak_ref_t* weak_ref);
void mjs_gc_process_weak_refs(mjs_gc_t* gc);

/* Statistics and monitoring */
mjs_gc_stats_t mjs_gc_get_stats(mjs_gc_t* gc);
size_t mjs_gc_get_memory_usage(mjs_gc_t* gc);
size_t mjs_gc_get_object_count(mjs_gc_t* gc);
void mjs_gc_print_stats(mjs_gc_t* gc);

/* Configuration */
void mjs_gc_set_memory_limit(mjs_gc_t* gc, size_t limit);
void mjs_gc_set_young_threshold(mjs_gc_t* gc, size_t threshold);
void mjs_gc_set_old_threshold(mjs_gc_t* gc, size_t threshold);
void mjs_gc_set_incremental_mode(mjs_gc_t* gc, bool enabled);
void mjs_gc_set_incremental_step_size(mjs_gc_t* gc, size_t step_size);

/* Debugging */
void mjs_gc_dump_heap(mjs_gc_t* gc);
void mjs_gc_verify_heap(mjs_gc_t* gc);
bool mjs_gc_is_valid_object(mjs_gc_t* gc, void* ptr);

/* Internal helpers */
static inline mjs_gc_object_t* mjs_gc_get_header(void* ptr) {
    return (mjs_gc_object_t*)((char*)ptr - sizeof(mjs_gc_object_t));
}

static inline void* mjs_gc_get_data(mjs_gc_object_t* header) {
    return (char*)header + sizeof(mjs_gc_object_t);
}

static inline bool mjs_gc_should_collect(mjs_gc_t* gc) {
    return (gc->young_generation.total_size >= gc->young_threshold) ||
           (gc->old_generation.total_size >= gc->old_threshold);
}

/* Macros for convenient allocation */
#define MJS_GC_ALLOC(gc, type, gc_type) \
    ((type*)mjs_gc_alloc(gc, sizeof(type), gc_type))

#define MJS_GC_ALLOC_ARRAY(gc, type, count, gc_type) \
    ((type*)mjs_gc_alloc(gc, sizeof(type) * (count), gc_type))

#define MJS_GC_FREE(gc, ptr) \
    mjs_gc_free_object(gc, ptr)

/* GC-safe macros for temporary root management */
#define MJS_GC_PROTECT(gc, obj) \
    do { mjs_gc_push_root(gc, (mjs_gc_object_t*)(obj)); } while(0)

#define MJS_GC_UNPROTECT(gc) \
    do { mjs_gc_pop_root(gc); } while(0)

#define MJS_GC_PROTECT_BLOCK(gc, obj, block) \
    do { \
        MJS_GC_PROTECT(gc, obj); \
        block; \
        MJS_GC_UNPROTECT(gc); \
    } while(0)

#endif /* MIKOJS_GC_H */