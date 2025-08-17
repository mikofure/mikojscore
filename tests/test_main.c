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
 * MikoJSCore Test Suite
 * Main test runner for all unit tests
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Test function declarations */
extern int test_lexer_run(void);
extern int test_parser_run(void);
extern int test_runtime_run(void);
extern int test_vm_run(void);
extern int test_gc_run(void);

/* Test statistics */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/* Test macros */
#define TEST_ASSERT(condition, message) do { \
    tests_run++; \
    if (condition) { \
        tests_passed++; \
        printf("  ✓ %s\n", message); \
    } else { \
        tests_failed++; \
        printf("  ✗ %s\n", message); \
    } \
} while(0)

#define TEST_SUITE_BEGIN(name) do { \
    printf("\n=== %s ===\n", name); \
} while(0)

#define TEST_SUITE_END() do { \
    printf("\n"); \
} while(0)

static void print_usage(const char* program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  --help      Show this help message\n");
    printf("  --lexer     Run only lexer tests\n");
    printf("  --parser    Run only parser tests\n");
    printf("  --runtime   Run only runtime tests\n");
    printf("  --vm        Run only VM tests\n");
    printf("  --gc        Run only GC tests\n");
    printf("  (no args)   Run all tests\n");
}

static void print_summary(void) {
    printf("\n=== Test Summary ===\n");
    printf("Total tests: %d\n", tests_run);
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    
    if (tests_failed == 0) {
        printf("\n🎉 All tests passed!\n");
    } else {
        printf("\n❌ %d test(s) failed.\n", tests_failed);
    }
}

int main(int argc, char* argv[]) {
    printf("MikoJSCore Test Suite\n");
    printf("=====================\n");
    
    bool run_all = true;
    bool run_lexer = false;
    bool run_parser = false;
    bool run_runtime = false;
    bool run_vm = false;
    bool run_gc = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--lexer") == 0) {
            run_all = false;
            run_lexer = true;
        } else if (strcmp(argv[i], "--parser") == 0) {
            run_all = false;
            run_parser = true;
        } else if (strcmp(argv[i], "--runtime") == 0) {
            run_all = false;
            run_runtime = true;
        } else if (strcmp(argv[i], "--vm") == 0) {
            run_all = false;
            run_vm = true;
        } else if (strcmp(argv[i], "--gc") == 0) {
            run_all = false;
            run_gc = true;
        } else {
            printf("Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    int exit_code = 0;
    
    // Run tests
    if (run_all || run_lexer) {
        if (test_lexer_run() != 0) {
            exit_code = 1;
        }
    }
    
    if (run_all || run_parser) {
        if (test_parser_run() != 0) {
            exit_code = 1;
        }
    }
    
    if (run_all || run_runtime) {
        if (test_runtime_run() != 0) {
            exit_code = 1;
        }
    }
    
    if (run_all || run_vm) {
        if (test_vm_run() != 0) {
            exit_code = 1;
        }
    }
    
    if (run_all || run_gc) {
        if (test_gc_run() != 0) {
            exit_code = 1;
        }
    }
    
    print_summary();
    
    return exit_code;
}

/* Export test macros for use in other test files */
int get_tests_run(void) { return tests_run; }
int get_tests_passed(void) { return tests_passed; }
int get_tests_failed(void) { return tests_failed; }

void reset_test_stats(void) {
    tests_run = 0;
    tests_passed = 0;
    tests_failed = 0;
}