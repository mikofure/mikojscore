# Test suite for MikoJSCore

# Find required packages
find_package(Threads REQUIRED)

# Test sources
set(TEST_SOURCES
    test_lexer.c
    test_parser.c
    test_runtime.c
    test_vm.c
    test_gc.c
    test_main.c
)

# Create test executable
add_executable(mikojs_tests ${TEST_SOURCES})

# Link with main library
target_link_libraries(mikojs_tests mikojs Threads::Threads)

# Set test properties
set_target_properties(mikojs_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
)

# Add tests
add_test(NAME mikojs_unit_tests COMMAND mikojs_tests)

# Individual test targets for debugging
add_test(NAME lexer_tests COMMAND mikojs_tests --lexer)
add_test(NAME parser_tests COMMAND mikojs_tests --parser)
add_test(NAME runtime_tests COMMAND mikojs_tests --runtime)
add_test(NAME vm_tests COMMAND mikojs_tests --vm)
add_test(NAME gc_tests COMMAND mikojs_tests --gc)