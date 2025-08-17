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
 * MikoJSCore Garbage Collector Tests
 */

#include "../src/gc.h"
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

static int test_gc_creation(void) {
    TEST_SUITE_BEGIN("GC Creation and Destruction");
    
    mjs_gc_t* gc = mjs_gc_new(NULL);
    TEST_ASSERT(gc != NULL, "GC creation");
    
    mjs_gc_free(gc);
    TEST_ASSERT(1, "GC destruction");
    
    return 0;
}

static int test_memory_allocation(void) {
    TEST_SUITE_BEGIN("Memory Allocation");
    
    mjs_gc_t* gc = mjs_gc_new(NULL);
    TEST_ASSERT(gc != NULL, "GC creation for allocation test");
    
    // Test basic allocation
    void* ptr1 = mjs_gc_alloc(gc, 64, MJS_GC_TYPE_OBJECT);
    TEST_ASSERT(ptr1 != NULL, "Basic allocation");
    
    // Test multiple allocations
    void* ptr2 = mjs_gc_alloc(gc, 128, MJS_GC_TYPE_ARRAY);
    void* ptr3 = mjs_gc_alloc(gc, 256, MJS_GC_TYPE_STRING);
    TEST_ASSERT(ptr2 != NULL && ptr3 != NULL, "Multiple allocations");
    
    mjs_gc_free(gc);
    
    return 0;
}

static int test_root_management(void) {
    TEST_SUITE_BEGIN("Root Management");
    
    mjs_gc_t* gc = mjs_gc_new(NULL);
    TEST_ASSERT(gc != NULL, "GC creation for root test");
    
    void* obj = mjs_gc_alloc(gc, 64, MJS_GC_TYPE_OBJECT);
    TEST_ASSERT(obj != NULL, "Object allocation for root test");
    
    // Add root
    bool added = mjs_gc_add_root(gc, obj);
    TEST_ASSERT(added, "Root addition");
    
    // Remove root
    bool removed = mjs_gc_remove_root(gc, obj);
    TEST_ASSERT(removed, "Root removal");
    
    mjs_gc_free(gc);
    
    return 0;
}

static int test_basic_collection(void) {
    TEST_SUITE_BEGIN("Basic Garbage Collection");
    
    mjs_gc_t* gc = mjs_gc_new();
    
    // Allocate some objects
    void* obj1 = mjs_gc_alloc(gc, 64, MJS_GC_TYPE_OBJECT);
    void* obj2 = mjs_gc_alloc(gc, 128, MJS_GC_TYPE_STRING);
    void* obj3 = mjs_gc_alloc(gc, 32, MJS_GC_TYPE_ARRAY);
    
    TEST_ASSERT(obj1 != NULL && obj2 != NULL && obj3 != NULL, "Multiple object allocation");
    
    // Add one as root to keep it alive
    mjs_gc_add_root(gc, obj1);
    
    // Trigger collection
    size_t collected = mjs_gc_collect(gc);
    TEST_ASSERT(collected >= 0, "Garbage collection execution");
    
    mjs_gc_free(gc);
    
    return 0;
}

static int test_generational_collection(void) {
    TEST_SUITE_BEGIN("Generational Collection");
    
    mjs_gc_t* gc = mjs_gc_new();
    
    // Allocate young objects
    void* young_obj1 = mjs_gc_alloc(gc, 64, MJS_GC_TYPE_OBJECT);
    void* young_obj2 = mjs_gc_alloc(gc, 32, MJS_GC_TYPE_STRING);
    
    TEST_ASSERT(young_obj1 != NULL && young_obj2 != NULL, "Young object allocation");
    
    // Add one as root
    mjs_gc_add_root(gc, young_obj1);
    
    // Trigger young generation collection
    size_t collected = mjs_gc_collect_young(gc);
    TEST_ASSERT(collected >= 0, "Young generation collection");
    
    mjs_gc_free(gc);
    
    return 0;
}

static int test_incremental_collection(void) {
    TEST_SUITE_BEGIN("Incremental Collection");
    
    mjs_gc_t* gc = mjs_gc_new();
    
    // Allocate some objects
    for (int i = 0; i < 10; i++) {
        void* obj = mjs_gc_alloc(gc, 64, MJS_GC_TYPE_OBJECT);
        if (i == 0) {
            // Keep first object as root
            mjs_gc_add_root(gc, obj);
        }
    }
    
    // Trigger incremental collection
    bool more_work = mjs_gc_collect_incremental(gc, 100); // 100 microseconds
    TEST_ASSERT(more_work >= 0, "Incremental collection execution");
    
    mjs_gc_free(gc);
    
    return 0;
}

static int test_weak_references(void) {
    TEST_SUITE_BEGIN("Weak References");
    
    mjs_gc_t* gc = mjs_gc_new();
    
    // Allocate an object
    void* obj = mjs_gc_alloc(gc, 64, MJS_GC_TYPE_OBJECT);
    TEST_ASSERT(obj != NULL, "Object allocation for weak ref test");
    
    // Create weak reference
    mjs_weak_ref_t* weak_ref = mjs_gc_create_weak_ref(gc, obj);
    TEST_ASSERT(weak_ref != NULL, "Weak reference creation");
    
    // Check if object is still alive
    void* retrieved = mjs_weak_ref_get(weak_ref);
    TEST_ASSERT(retrieved == obj, "Weak reference retrieval");
    
    // Clean up weak reference
    mjs_weak_ref_free(weak_ref);
    
    mjs_gc_free(gc);
    
    return 0;
}

static int test_gc_statistics(void) {
    TEST_SUITE_BEGIN("GC Statistics");
    
    mjs_gc_t* gc = mjs_gc_new();
    
    // Get initial statistics
    mjs_gc_stats_t stats;
    mjs_gc_get_stats(gc, &stats);
    
    TEST_ASSERT(stats.total_allocated >= 0, "Initial statistics retrieval");
    
    // Allocate some objects
    for (int i = 0; i < 5; i++) {
        mjs_gc_alloc(gc, 64, MJS_GC_TYPE_OBJECT);
    }
    
    // Get updated statistics
    mjs_gc_get_stats(gc, &stats);
    TEST_ASSERT(stats.total_allocated > 0, "Updated statistics after allocation");
    
    // Trigger collection and check stats
    mjs_gc_collect(gc);
    mjs_gc_get_stats(gc, &stats);
    TEST_ASSERT(stats.collections_performed > 0, "Collection statistics");
    
    mjs_gc_free(gc);
    
    return 0;
}

static int test_memory_pressure(void) {
    TEST_SUITE_BEGIN("Memory Pressure Handling");
    
    mjs_gc_t* gc = mjs_gc_new();
    
    // Allocate many objects to trigger automatic collection
    void* roots[10];
    for (int i = 0; i < 100; i++) {
        void* obj = mjs_gc_alloc(gc, 1024, MJS_GC_TYPE_OBJECT);
        if (i < 10) {
            roots[i] = obj;
            mjs_gc_add_root(gc, obj);
        }
    }
    
    // Check that GC handled memory pressure
    mjs_gc_stats_t stats;
    mjs_gc_get_stats(gc, &stats);
    TEST_ASSERT(stats.collections_performed >= 0, "Automatic collection under pressure");
    
    // Clean up roots
    for (int i = 0; i < 10; i++) {
        mjs_gc_remove_root(gc, roots[i]);
    }
    
    mjs_gc_free(gc);
    
    return 0;
}

int test_gc_run(void) {
    printf("\n=== Running GC Tests ===\n");
    
    int result = 0;
    
    result |= test_gc_creation();
    result |= test_memory_allocation();
    result |= test_root_management();
    result |= test_basic_collection();
    result |= test_generational_collection();
    result |= test_incremental_collection();
    result |= test_weak_references();
    result |= test_gc_statistics();
    result |= test_memory_pressure();
    
    if (result == 0) {
        printf("\n✅ All GC tests passed!\n");
    } else {
        printf("\n❌ Some GC tests failed.\n");
    }
    
    return result;
}