cmake_minimum_required(VERSION 3.16)
project(MikoJSCore VERSION 0.1.0 LANGUAGES C CXX)

# Set C and C++ standards
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler-specific flags
if(MSVC)
    add_compile_options(/W4)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Include directories
include_directories(include)
include_directories(src)

# Source files
set(MIKOJS_SOURCES
    src/array.c
    src/compiler.c
    src/gc.c
    src/lexer.c
    src/object.c
    src/parser.c
    src/runtime.c
    src/string.c
    src/vm.c
)

# Header files
set(MIKOJS_HEADERS
    include/mikojs.h
    src/gc.h
    src/lexer.h
    src/mikojs_internal.h
    src/parser.h
    src/vm.h
)

# ============================================================================
# Library Target
# ============================================================================

# Create the main MikoJS library
add_library(mikojs STATIC ${MIKOJS_SOURCES} ${MIKOJS_HEADERS})

# Set library properties
set_target_properties(mikojs PROPERTIES
    OUTPUT_NAME "mikojs"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Library include directories
target_include_directories(mikojs PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(mikojs PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ============================================================================
# Executable Target
# ============================================================================

# # Create executable for testing/examples
# add_executable(mikojs_test
#     examples/main.c
# )

# # Link executable with library
# target_link_libraries(mikojs_test PRIVATE mikojs)

# # Set executable properties
# set_target_properties(mikojs_test PROPERTIES
#     OUTPUT_NAME "mikojs_test"
#     RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
# )

# Create shell executable
add_executable(mikojs_shell
    shell/shell.c
)

# Link shell executable with library
target_link_libraries(mikojs_shell PRIVATE mikojs)

# Set shell executable properties
set_target_properties(mikojs_shell PROPERTIES
    OUTPUT_NAME "miko"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Install targets
install(TARGETS mikojs
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# ============================================================================
# Testing Configuration
# ============================================================================

# Enable CTest framework for running unit tests
# This allows using 'ctest' command to run all tests or specific test suites
# enable_testing()

# Add the tests subdirectory which contains:
# - test_gc.c: Garbage collector functionality tests
# - test_lexer.c: JavaScript tokenizer/lexer tests
# - test_parser.c: JavaScript parser and AST generation tests
# - test_runtime.c: Runtime value creation and manipulation tests
# - test_vm.c: Virtual machine bytecode execution tests
# - test_main.c: Main test runner that orchestrates all test suites
#
# The tests subdirectory has its own CMakeLists.txt that defines:
# - Individual test executables for each component
# - Test dependencies and linking with the main mikojs library
# - CTest integration for automated testing
# 
# To run tests:
# - Build: cmake --build . --target mikojs_tests
# - Run all: ctest
# - Run specific: ctest -R test_lexer
# - Verbose output: ctest -V
# add_subdirectory(tests)