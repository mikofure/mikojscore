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
 * MikoJS Interactive Shell
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../include/mikojs.h"
#include "../src/mikojs_internal.h"

#define MAX_INPUT_SIZE 4096
#define PROMPT "miko> "

// Function to print welcome message
void print_welcome() {
    printf("MikoJS Interactive Shell\n");
    printf("Version 1.0.0\n");
    printf("Type 'exit' or 'quit' to exit, 'help' for help\n\n");
}

// Function to print help message
void print_help() {
    printf("MikoJS Shell Commands:\n");
    printf("  help          - Show this help message\n");
    printf("  exit, quit    - Exit the shell\n");
    printf("  clear         - Clear the screen\n");
    printf("  .gc           - Force garbage collection\n");
    printf("  .stats        - Show runtime statistics\n");
    printf("\nJavaScript expressions and statements are executed directly.\n\n");
}

// Function to clear screen (cross-platform)
void clear_screen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// Function to trim whitespace from string
char* trim_whitespace(char* str) {
    char* end;
    
    // Trim leading space
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') str++;
    
    if (*str == 0) return str;
    
    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) end--;
    
    end[1] = '\0';
    return str;
}

// Function to handle shell commands
bool handle_shell_command(const char* input, mjs_context_t* ctx) {
    if (strcmp(input, "help") == 0) {
        print_help();
        return true;
    }
    
    if (strcmp(input, "exit") == 0 || strcmp(input, "quit") == 0) {
        printf("Goodbye!\n");
        return false;
    }
    
    if (strcmp(input, "clear") == 0) {
        clear_screen();
        return true;
    }
    
    if (strcmp(input, ".gc") == 0) {
        if (ctx) {
            mjs_gc(ctx);
            printf("Garbage collection completed\n");
        } else {
            printf("Context not available\n");
        }
        return true;
    }
    
    if (strcmp(input, ".stats") == 0) {
        if (ctx) {
            size_t memory_usage = mjs_get_memory_usage(ctx);
            printf("Runtime Statistics:\n");
            printf("  Memory usage: %zu bytes\n", memory_usage);
        } else {
            printf("Context not available\n");
        }
        return true;
    }
    
    return false; // Not a shell command
}

// Function to execute JavaScript code
void execute_javascript(const char* code, mjs_context_t* ctx) {
    if (!ctx) {
        printf("Error: Context not initialized\n");
        return;
    }
    
    mjs_value_t result;
    mjs_result_t eval_result = mjs_eval(ctx, code, "<shell>", &result);
    
    if (eval_result != MJS_OK) {
        const char* error_msg = mjs_get_error_message(ctx);
        printf("Error: %s\n", error_msg ? error_msg : "Unknown error");
        mjs_clear_error(ctx);
    } else {
        // Print the result if it's not undefined
        if (!mjs_is_undefined(result)) {
            const char* str_result = mjs_to_string(ctx, result);
            if (str_result) {
                printf("%s\n", str_result);
            }
        }
    }
}

// Main REPL loop
void repl_loop(mjs_context_t* ctx) {
    char input[MAX_INPUT_SIZE];
    char* trimmed_input;
    
    while (true) {
        printf(PROMPT);
        fflush(stdout);
        
        // Read input
        if (!fgets(input, sizeof(input), stdin)) {
            printf("\n");
            break;
        }
        
        // Remove newline and trim whitespace
        input[strcspn(input, "\n")] = 0;
        trimmed_input = trim_whitespace(input);
        
        // Skip empty lines
        if (strlen(trimmed_input) == 0) {
            continue;
        }
        
        // Handle shell commands
        if (handle_shell_command(trimmed_input, ctx)) {
            continue;
        }
        
        // Execute as JavaScript
        execute_javascript(trimmed_input, ctx);
    }
}

int main(int argc, char* argv[]) {
    // Initialize runtime
    mjs_runtime_t* runtime = mjs_new_runtime();
    if (!runtime) {
        fprintf(stderr, "Error: Failed to initialize MikoJS runtime\n");
        return 1;
    }
    
    // Create context
    mjs_context_t* ctx = mjs_new_context(runtime);
    if (!ctx) {
        fprintf(stderr, "Error: Failed to create MikoJS context\n");
        mjs_free_runtime(runtime);
        return 1;
    }
    
    // Print welcome message
    print_welcome();
    
    // Check if file argument is provided
    if (argc > 1) {
        // Execute file mode
        mjs_value_t result;
        mjs_result_t eval_result = mjs_eval_file(ctx, argv[1], &result);
        
        if (eval_result != MJS_OK) {
            const char* error_msg = mjs_get_error_message(ctx);
            fprintf(stderr, "Error executing file '%s': %s\n", argv[1], 
                    error_msg ? error_msg : "Unknown error");
            mjs_free_context(ctx);
            mjs_free_runtime(runtime);
            return 1;
        }
        
        printf("File '%s' executed successfully\n", argv[1]);
    } else {
        // Interactive mode
        repl_loop(ctx);
    }
    
    // Cleanup
    mjs_free_context(ctx);
    mjs_free_runtime(runtime);
    return 0;
}