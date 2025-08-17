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
 * MikoJSCore Garbage Collector Implementation
 * Generational mark-and-sweep garbage collector with incremental collection
 */

#include "gc.h"
#include "mikojs_internal.h"

/* Internal GC constants */
#define GC_INITIAL_HEAP_SIZE (1024 * 1024)  // 1MB
#define GC_GROWTH_FACTOR 2
#define GC_COLLECTION_THRESHOLD 0.8
#define GC_YOUNG_GENERATION_SIZE (256 * 1024)  // 256KB
#define GC_INCREMENTAL_STEP_SIZE 100

/* GC object header manipulation */
#define GC_HEADER_SIZE sizeof(mjs_gc_object_header_t)
#define GC_OBJECT_TO_HEADER(obj) ((mjs_gc_object_header_t*)((char*)(obj) - GC_HEADER_SIZE))
#define GC_HEADER_TO_OBJECT(header) ((void*)((char*)(header) + GC_HEADER_SIZE))

/* Mark bits */
#define GC_MARK_WHITE 0
#define GC_MARK_GRAY  1
#define GC_MARK_BLACK 2

/* Forward declarations */
static void gc_mark_object(mjs_gc_t* gc, void* obj);
static void gc_mark_roots(mjs_gc_t* gc);
static void gc_sweep(mjs_gc_t* gc);
static void gc_compact(mjs_gc_t* gc);
static bool gc_should_collect(mjs_gc_t* gc);
static void gc_update_statistics(mjs_gc_t* gc);

/* GC creation and destruction */
mjs_gc_t* mjs_gc_new(mjs_runtime_t* runtime) {
    mjs_gc_t* gc = MJS_MALLOC(sizeof(mjs_gc_t));
    if (!gc) return NULL;
    
    memset(gc, 0, sizeof(mjs_gc_t));
    
    // Store runtime reference
    gc->runtime = runtime;
    
    // Initialize heap
    size_t heap_size = GC_INITIAL_HEAP_SIZE;
    gc->heap = MJS_MALLOC(heap_size);
    if (!gc->heap) {
        MJS_FREE(gc);
        return NULL;
    }
    gc->heap_size = heap_size;
    gc->heap_used = 0;
    gc->heap_ptr = gc->heap;
    
    // Initialize generations
    gc->young_generation.objects = NULL;
    gc->young_generation.object_count = 0;
    gc->young_generation.total_size = 0;
    gc->young_generation.threshold = GC_YOUNG_GENERATION_SIZE;
    
    gc->old_generation.objects = NULL;
    gc->old_generation.object_count = 0;
    gc->old_generation.total_size = 0;
    gc->old_generation.threshold = heap_size;
    
    // Initialize roots
    gc->roots = NULL;
    gc->root_count = 0;
    gc->root_capacity = 0;
    
    // Initialize gray stack for marking
    gc->gray_stack = NULL;
    gc->gray_count = 0;
    gc->gray_capacity = 0;
    
    // Initialize weak references
    gc->weak_refs = NULL;
    
    // Initialize statistics
    gc->stats.total_collections = 0;
    gc->stats.total_allocations = 0;
    gc->stats.total_deallocations = 0;
    gc->stats.total_bytes_allocated = 0;
    gc->stats.total_bytes_freed = 0;
    gc->stats.total_collection_time = 0;
    gc->stats.peak_memory_usage = 0;
    gc->stats.last_collection_time = 0;
    
    // Initialize configuration
    gc->config.incremental = true;
    gc->config.generational = true;
    gc->config.collection_threshold = GC_COLLECTION_THRESHOLD;
    gc->config.max_heap_size = 0; // Unlimited
    
    // Initialize state
    gc->state = GC_STATE_IDLE;
    gc->incremental_step = 0;
    
    return gc;
}

void mjs_gc_free(mjs_gc_t* gc) {
    if (!gc) return;
    
    // Free all objects
    mjs_gc_collect(gc);
    
    // Free heap
    if (gc->heap) {
        MJS_FREE(gc->heap);
    }
    
    // Free roots array
    if (gc->roots) {
        MJS_FREE(gc->roots);
    }
    
    // Free gray stack
    if (gc->gray_stack) {
        MJS_FREE(gc->gray_stack);
    }
    
    // Free weak references
    mjs_weak_ref_t* weak_ref = gc->weak_refs;
    while (weak_ref) {
        mjs_weak_ref_t* next = weak_ref->next;
        MJS_FREE(weak_ref);
        weak_ref = next;
    }
    
    MJS_FREE(gc);
}

/* Memory allocation */
void* mjs_gc_alloc(mjs_gc_t* gc, size_t size, mjs_gc_object_type_t type) {
    if (!gc || size == 0) return NULL;
    
    // Check if collection is needed
    if (gc_should_collect(gc)) {
        mjs_gc_collect(gc);
    }
    
    // Calculate total size including header
    size_t total_size = size + GC_HEADER_SIZE;
    
    // Check if we have enough space
    if (gc->heap_used + total_size > gc->heap_size) {
        // Try to collect first
        mjs_gc_collect(gc);
        
        // If still not enough space, try to expand heap
        if (gc->heap_used + total_size > gc->heap_size) {
            size_t new_heap_size = gc->heap_size * GC_GROWTH_FACTOR;
            if (gc->config.max_heap_size > 0 && new_heap_size > gc->config.max_heap_size) {
                return NULL; // Out of memory
            }
            
            void* new_heap = MJS_REALLOC(gc->heap, new_heap_size);
            if (!new_heap) {
                return NULL; // Out of memory
            }
            
            gc->heap = new_heap;
            gc->heap_size = new_heap_size;
            gc->heap_ptr = (char*)gc->heap + gc->heap_used;
        }
    }
    
    // Allocate object
    mjs_gc_object_header_t* header = (mjs_gc_object_header_t*)gc->heap_ptr;
    void* object = GC_HEADER_TO_OBJECT(header);
    
    // Initialize header
    header->type = type;
    header->size = size;
    header->mark = GC_MARK_WHITE;
    header->generation = 0; // Start in young generation
    header->next = NULL;
    
    // Add to young generation
    header->next = gc->young_generation.objects;
    gc->young_generation.objects = header;
    gc->young_generation.size += total_size;
    
    // Update heap pointers
    gc->heap_used += total_size;
    gc->heap_ptr = (char*)gc->heap_ptr + total_size;
    
    // Update statistics
    gc->stats.total_allocations++;
    gc->stats.total_bytes_allocated += total_size;
    
    return object;
}

void mjs_gc_free_object(mjs_gc_t* gc, void* obj) {
    if (!gc || !obj) return;
    
    mjs_gc_object_header_t* header = GC_OBJECT_TO_HEADER(obj);
    
    // Remove from generation list
    mjs_gc_generation_t* gen = (header->generation == 0) ? 
        &gc->young_generation : &gc->old_generation;
    
    if (gen->objects == header) {
        gen->objects = header->next;
    } else {
        mjs_gc_object_header_t* prev = gen->objects;
        while (prev && prev->next != header) {
            prev = prev->next;
        }
        if (prev) {
            prev->next = header->next;
        }
    }
    
    gen->size -= (header->size + GC_HEADER_SIZE);
    
    // Update statistics
    gc->stats.total_deallocations++;
    gc->stats.total_bytes_freed += (header->size + GC_HEADER_SIZE);
    
    // Note: We don't actually free the memory here in a real GC,
    // as it would fragment the heap. Instead, we mark it as free
    // and it will be reclaimed during compaction.
}

/* Root management */
bool mjs_gc_add_root(mjs_gc_t* gc, void* root) {
    if (!gc || !root) return false;
    
    // Expand roots array if needed
    if (gc->root_count >= gc->root_capacity) {
        size_t new_capacity = gc->root_capacity == 0 ? 16 : gc->root_capacity * 2;
        void** new_roots = MJS_REALLOC(gc->roots, sizeof(void*) * new_capacity);
        if (!new_roots) return false;
        
        gc->roots = new_roots;
        gc->root_capacity = new_capacity;
    }
    
    gc->roots[gc->root_count++] = root;
    return true;
}

bool mjs_gc_remove_root(mjs_gc_t* gc, void* root) {
    if (!gc || !root) return false;
    
    for (size_t i = 0; i < gc->root_count; i++) {
        if (gc->roots[i] == root) {
            // Move last element to this position
            gc->roots[i] = gc->roots[gc->root_count - 1];
            gc->root_count--;
            return true;
        }
    }
    
    return false;
}

/* Collection triggers */
bool mjs_gc_collect(mjs_gc_t* gc) {
    if (!gc) return false;
    
    clock_t start_time = clock();
    
    // Mark phase
    gc->state = GC_STATE_MARKING;
    gc_mark_roots(gc);
    
    // Process gray stack
    while (gc->gray_count > 0) {
        mjs_gc_object_header_t* obj = gc->gray_stack[--gc->gray_count];
        gc_mark_object(gc, GC_HEADER_TO_OBJECT(obj));
    }
    
    // Sweep phase
    gc->state = GC_STATE_SWEEPING;
    gc_sweep(gc);
    
    // Compact phase (optional)
    if (gc->config.compact) {
        gc->state = GC_STATE_COMPACTING;
        gc_compact(gc);
    }
    
    gc->state = GC_STATE_IDLE;
    
    // Update statistics
    clock_t end_time = clock();
    gc->stats.collection_time += (end_time - start_time);
    gc->stats.collections++;
    
    gc_update_statistics(gc);
    
    return true;
}

bool mjs_gc_collect_young(mjs_gc_t* gc) {
    if (!gc || !gc->config.generational) {
        return mjs_gc_collect(gc);
    }
    
    // Minor collection - only collect young generation
    clock_t start_time = clock();
    
    gc->state = GC_STATE_MARKING;
    
    // Mark roots that point to young generation
    for (size_t i = 0; i < gc->root_count; i++) {
        void* obj = gc->roots[i];
        if (obj) {
            mjs_gc_object_header_t* header = GC_OBJECT_TO_HEADER(obj);
            if (header->generation == 0) {
                gc_mark_object(gc, obj);
            }
        }
    }
    
    // Mark objects in old generation that reference young objects
    mjs_gc_object_header_t* old_obj = gc->old_generation.objects;
    while (old_obj) {
        // TODO: Implement remembered set or card marking
        // For now, we'll skip this optimization
        old_obj = old_obj->next;
    }
    
    // Process gray stack
    while (gc->gray_count > 0) {
        mjs_gc_object_header_t* obj = gc->gray_stack[--gc->gray_count];
        if (obj->generation == 0) {
            gc_mark_object(gc, GC_HEADER_TO_OBJECT(obj));
        }
    }
    
    // Sweep young generation
    gc->state = GC_STATE_SWEEPING;
    mjs_gc_object_header_t* young_obj = gc->young_generation.objects;
    mjs_gc_object_header_t* prev = NULL;
    
    while (young_obj) {
        mjs_gc_object_header_t* next = young_obj->next;
        
        if (young_obj->mark == GC_MARK_WHITE) {
            // Object is garbage - remove from list
            if (prev) {
                prev->next = next;
            } else {
                gc->young_generation.objects = next;
            }
            
            gc->young_generation.size -= (young_obj->size + GC_HEADER_SIZE);
            gc->stats.objects_freed++;
            gc->stats.bytes_freed += (young_obj->size + GC_HEADER_SIZE);
        } else {
            // Object survived - promote to old generation if it's old enough
            young_obj->generation++;
            if (young_obj->generation >= 2) {
                // Move to old generation
                if (prev) {
                    prev->next = next;
                } else {
                    gc->young_generation.objects = next;
                }
                
                young_obj->next = gc->old_generation.objects;
                gc->old_generation.objects = young_obj;
                
                size_t obj_size = young_obj->size + GC_HEADER_SIZE;
                gc->young_generation.size -= obj_size;
                gc->old_generation.size += obj_size;
            } else {
                prev = young_obj;
            }
            
            // Reset mark for next collection
            young_obj->mark = GC_MARK_WHITE;
        }
        
        young_obj = next;
    }
    
    gc->state = GC_STATE_IDLE;
    
    // Update statistics
    clock_t end_time = clock();
    gc->stats.collection_time += (end_time - start_time);
    gc->stats.collections++;
    
    return true;
}

bool mjs_gc_collect_incremental(mjs_gc_t* gc, uint64_t time_limit_us) {
    if (!gc || !gc->config.incremental) {
        return mjs_gc_collect(gc);
    }
    
    // Perform incremental collection step
    switch (gc->state) {
        case GC_STATE_IDLE:
            gc->state = GC_STATE_MARKING;
            gc->incremental_step = 0;
            gc_mark_roots(gc);
            break;
            
        case GC_STATE_MARKING: {
            // Process some objects from gray stack
            size_t processed = 0;
            while (gc->gray_count > 0 && processed < GC_INCREMENTAL_STEP_SIZE) {
                mjs_gc_object_header_t* obj = gc->gray_stack[--gc->gray_count];
                gc_mark_object(gc, GC_HEADER_TO_OBJECT(obj));
                processed++;
            }
            
            if (gc->gray_count == 0) {
                gc->state = GC_STATE_SWEEPING;
                gc->incremental_step = 0;
            }
            break;
        }
        
        case GC_STATE_SWEEPING:
            // TODO: Implement incremental sweeping
            gc_sweep(gc);
            gc->state = GC_STATE_IDLE;
            gc->stats.collections++;
            break;
            
        case GC_STATE_COMPACTING:
            // TODO: Implement incremental compaction
            gc_compact(gc);
            gc->state = GC_STATE_IDLE;
            break;
    }
    
    return true;
}

/* Marking implementation */
static void gc_mark_roots(mjs_gc_t* gc) {
    for (size_t i = 0; i < gc->root_count; i++) {
        void* obj = gc->roots[i];
        if (obj) {
            gc_mark_object(gc, obj);
        }
    }
}

static void gc_mark_object(mjs_gc_t* gc, void* obj) {
    if (!obj) return;
    
    mjs_gc_object_header_t* header = GC_OBJECT_TO_HEADER(obj);
    
    // Skip if already marked
    if (header->mark != GC_MARK_WHITE) return;
    
    // Mark as gray (being processed)
    header->mark = GC_MARK_GRAY;
    
    // Add to gray stack for processing
    if (gc->gray_count >= gc->gray_capacity) {
        size_t new_capacity = gc->gray_capacity == 0 ? 256 : gc->gray_capacity * 2;
        mjs_gc_object_header_t** new_stack = MJS_REALLOC(gc->gray_stack, 
            sizeof(mjs_gc_object_header_t*) * new_capacity);
        if (!new_stack) return; // Out of memory during GC
        
        gc->gray_stack = new_stack;
        gc->gray_capacity = new_capacity;
    }
    
    gc->gray_stack[gc->gray_count++] = header;
    
    // Mark object's children based on type
    switch (header->type) {
        case GC_TYPE_STRING:
            // Strings have no references
            break;
            
        case GC_TYPE_OBJECT: {
            mjs_object_t* object = (mjs_object_t*)obj;
            
            // Mark prototype
            if (object->prototype) {
                gc_mark_object(gc, object->prototype);
            }
            
            // Mark properties
            mjs_property_t* prop = object->properties;
            while (prop) {
                if (prop->key) {
                    gc_mark_object(gc, prop->key);
                }
                // TODO: Mark property value if it's an object
                prop = prop->next;
            }
            break;
        }
        
        case GC_TYPE_ARRAY: {
            mjs_array_t* array = (mjs_array_t*)obj;
            
            // Mark array elements
            for (size_t i = 0; i < array->length; i++) {
                // TODO: Mark array element if it's an object
            }
            break;
        }
        
        case GC_TYPE_FUNCTION: {
            mjs_function_t* function = (mjs_function_t*)obj;
            
            // Mark function name
            if (function->name) {
                gc_mark_object(gc, function->name);
            }
            
            // Mark closure variables
            // TODO: Implement closure marking
            break;
        }
        
        default:
            break;
    }
    
    // Mark as black (fully processed)
    header->mark = GC_MARK_BLACK;
}

/* Sweeping implementation */
static void gc_sweep(mjs_gc_t* gc) {
    // Sweep young generation
    mjs_gc_object_header_t* obj = gc->young_generation.objects;
    mjs_gc_object_header_t* prev = NULL;
    
    while (obj) {
        mjs_gc_object_header_t* next = obj->next;
        
        if (obj->mark == GC_MARK_WHITE) {
            // Object is garbage
            if (prev) {
                prev->next = next;
            } else {
                gc->young_generation.objects = next;
            }
            
            gc->young_generation.size -= (obj->size + GC_HEADER_SIZE);
            gc->stats.objects_freed++;
            gc->stats.bytes_freed += (obj->size + GC_HEADER_SIZE);
        } else {
            // Reset mark for next collection
            obj->mark = GC_MARK_WHITE;
            prev = obj;
        }
        
        obj = next;
    }
    
    // Sweep old generation
    obj = gc->old_generation.objects;
    prev = NULL;
    
    while (obj) {
        mjs_gc_object_header_t* next = obj->next;
        
        if (obj->mark == GC_MARK_WHITE) {
            // Object is garbage
            if (prev) {
                prev->next = next;
            } else {
                gc->old_generation.objects = next;
            }
            
            gc->old_generation.size -= (obj->size + GC_HEADER_SIZE);
            gc->stats.objects_freed++;
            gc->stats.bytes_freed += (obj->size + GC_HEADER_SIZE);
        } else {
            // Reset mark for next collection
            obj->mark = GC_MARK_WHITE;
            prev = obj;
        }
        
        obj = next;
    }
    
    // Process weak references
    mjs_weak_ref_t* weak_ref = gc->weak_refs;
    mjs_weak_ref_t* prev_weak = NULL;
    
    while (weak_ref) {
        mjs_weak_ref_t* next_weak = weak_ref->next;
        
        if (weak_ref->object) {
            mjs_gc_object_header_t* header = GC_OBJECT_TO_HEADER(weak_ref->object);
            if (header->mark == GC_MARK_WHITE) {
                // Referenced object was collected
                weak_ref->object = NULL;
                if (weak_ref->callback) {
                    weak_ref->callback(weak_ref->userdata);
                }
            }
        }
        
        if (!weak_ref->object) {
            // Remove dead weak reference
            if (prev_weak) {
                prev_weak->next = next_weak;
            } else {
                gc->weak_refs = next_weak;
            }
            MJS_FREE(weak_ref);
        } else {
            prev_weak = weak_ref;
        }
        
        weak_ref = next_weak;
    }
}

/* Compaction implementation */
static void gc_compact(mjs_gc_t* gc) {
    // TODO: Implement heap compaction
    // This is a complex operation that involves:
    // 1. Moving live objects to eliminate fragmentation
    // 2. Updating all pointers to moved objects
    // 3. Updating the heap pointer
}

/* Collection heuristics */
static bool gc_should_collect(mjs_gc_t* gc) {
    if (!gc) return false;
    
    // Check heap usage threshold
    double usage_ratio = (double)gc->heap_used / gc->heap_size;
    if (usage_ratio >= gc->config.collection_threshold) {
        return true;
    }
    
    // Check young generation size
    if (gc->config.generational && 
        gc->young_generation.size >= gc->young_generation.capacity) {
        return true;
    }
    
    return false;
}

/* Statistics and configuration */
static void gc_update_statistics(mjs_gc_t* gc) {
    gc->stats.heap_size = gc->heap_size;
    gc->stats.heap_used = gc->heap_used;
    gc->stats.young_generation_size = gc->young_generation.size;
    gc->stats.old_generation_size = gc->old_generation.size;
}

mjs_gc_stats_t mjs_gc_get_stats(mjs_gc_t* gc) {
    if (!gc) {
        mjs_gc_stats_t empty_stats = {0};
        return empty_stats;
    }
    
    return gc->stats;
}

void mjs_gc_set_config(mjs_gc_t* gc, mjs_gc_config_t config) {
    if (gc) {
        gc->config = config;
    }
}

mjs_gc_config_t mjs_gc_get_config(mjs_gc_t* gc) {
    if (!gc) {
        mjs_gc_config_t empty_config = {0};
        return empty_config;
    }
    
    return gc->config;
}

size_t mjs_gc_get_memory_usage(mjs_gc_t* gc) {
    if (!gc) return 0;
    
    return gc->young_generation.total_size + gc->old_generation.total_size;
}

/* Weak references */
mjs_weak_ref_t* mjs_gc_create_weak_ref(mjs_gc_t* gc, void* target) {
    if (!gc || !target) return NULL;
    
    mjs_weak_ref_t* weak_ref = MJS_MALLOC(sizeof(mjs_weak_ref_t));
    if (!weak_ref) return NULL;
    
    weak_ref->object = target;
    weak_ref->callback = NULL;
    weak_ref->callback_data = NULL;
    weak_ref->userdata = NULL;
    weak_ref->cleared = false;
    weak_ref->next = gc->weak_refs;
    
    gc->weak_refs = weak_ref;
    
    return weak_ref;
}

void mjs_gc_destroy_weak_ref(mjs_gc_t* gc, mjs_weak_ref_t* weak_ref) {
    if (!gc || !weak_ref) return;
    
    // Remove from list
    if (gc->weak_refs == weak_ref) {
        gc->weak_refs = weak_ref->next;
    } else {
        mjs_weak_ref_t* current = gc->weak_refs;
        while (current && current->next != weak_ref) {
            current = current->next;
        }
        if (current) {
            current->next = weak_ref->next;
        }
    }
    
    MJS_FREE(weak_ref);
}

void* mjs_weak_ref_get(mjs_weak_ref_t* weak_ref) {
    return weak_ref ? weak_ref->object : NULL;
}

/* Debug utilities */
void mjs_gc_dump_heap(mjs_gc_t* gc) {
    if (!gc) return;
    
    printf("=== GC Heap Dump ===\n");
    printf("Heap size: %zu bytes\n", gc->heap_size);
    printf("Heap used: %zu bytes\n", gc->heap_used);
    printf("Young generation: %zu bytes\n", gc->young_generation.size);
    printf("Old generation: %zu bytes\n", gc->old_generation.size);
    printf("Collections: %zu\n", gc->stats.collections);
    printf("Objects allocated: %zu\n", gc->stats.objects_allocated);
    printf("Objects freed: %zu\n", gc->stats.objects_freed);
    printf("Bytes allocated: %zu\n", gc->stats.bytes_allocated);
    printf("Bytes freed: %zu\n", gc->stats.bytes_freed);
    printf("Collection time: %ld ms\n", gc->stats.collection_time);
    printf("==================\n");
}

void mjs_gc_dump_objects(mjs_gc_t* gc) {
    if (!gc) return;
    
    printf("=== GC Objects ===\n");
    
    printf("Young generation objects:\n");
    mjs_gc_object_header_t* obj = gc->young_generation.objects;
    while (obj) {
        printf("  Object: %p, Type: %d, Size: %zu, Mark: %d\n", 
               GC_HEADER_TO_OBJECT(obj), obj->type, obj->size, obj->mark);
        obj = obj->next;
    }
    
    printf("Old generation objects:\n");
    obj = gc->old_generation.objects;
    while (obj) {
        printf("  Object: %p, Type: %d, Size: %zu, Mark: %d\n", 
               GC_HEADER_TO_OBJECT(obj), obj->type, obj->size, obj->mark);
        obj = obj->next;
    }
    
    printf("==================\n");
}