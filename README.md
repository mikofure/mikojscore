# MikoJS - JavaScript Engine

[![CI/CD Build and Test](https://github.com/username/mikojsc/actions/workflows/ci.yml/badge.svg)](https://github.com/username/mikojsc/actions/workflows/ci.yml)

MikoJS is a lightweight JavaScript engine written in C, featuring a complete implementation with lexer, parser, virtual machine, and garbage collector.

## Features

- **Lexical Analysis**: Complete JavaScript tokenizer with support for all standard tokens
- **Parser**: Recursive descent parser generating Abstract Syntax Trees (AST)
- **Virtual Machine**: Bytecode interpreter for efficient JavaScript execution
- **Garbage Collector**: Automatic memory management with generational and incremental collection
- **Runtime**: Complete JavaScript runtime with value types, objects, and arrays
- **Shell**: Interactive JavaScript REPL for testing and development

## Building

### Prerequisites

- CMake 3.10 or higher
- C compiler (GCC, Clang, or MSVC)
- Git

### Build Instructions

#### Using CMake (Recommended)

```bash
# Clone the repository
git clone <repository-url>
cd mikojsc

# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
cmake --build . --config Release
```

#### Using Make (Linux/WSL)

```bash
make
# or for WSL
make -f Makefile.wsl
```

## Testing

The project includes comprehensive unit tests for all components:

- `test_gc.c` - Garbage collector functionality tests
- `test_lexer.c` - JavaScript tokenizer/lexer tests
- `test_parser.c` - JavaScript parser and AST generation tests
- `test_runtime.c` - Runtime value creation and manipulation tests
- `test_vm.c` - Virtual machine bytecode execution tests
- `test_main.c` - Main test runner

### Running Tests

```bash
# Build tests
cmake --build . --target mikojs_tests

# Run all tests
ctest

# Run specific test suite
ctest -R test_lexer

# Run with verbose output
ctest -V
```

## CI/CD

The project uses GitHub Actions for continuous integration and deployment:

- **Multi-platform testing**: Builds and tests on both Linux (Ubuntu) and Windows
- **Automated testing**: Runs complete test suite on every push and pull request
- **Artifact generation**: Creates release artifacts for the main branch
- **Cross-compiler support**: Ensures compatibility across different build environments

### Workflow Triggers

- Push to `main` or `develop` branches
- Pull requests to `main` branch

## Project Structure

```
mikojsc/
├── include/           # Public header files
│   └── mikojs.h      # Main API header
├── src/              # Source code
│   ├── lexer.c/h     # Lexical analyzer
│   ├── parser.c/h    # Parser and AST
│   ├── vm.c/h        # Virtual machine
│   ├── gc.c/h        # Garbage collector
│   ├── runtime.c     # Runtime system
│   └── ...           # Other components
├── shell/            # Interactive shell
│   └── shell.c       # REPL implementation
├── tests/            # Unit tests
│   ├── test_*.c      # Individual test files
│   └── CMakeLists.txt # Test configuration
├── lib/              # JavaScript standard library
└── .github/workflows/ # CI/CD configuration
```

## Usage

### Interactive Shell

```bash
# Run the interactive JavaScript shell
./build/shell/mikojs
```

### Embedding in C Applications

```c
#include "mikojs.h"

int main() {
    // Create JavaScript context
    mjs_context_t* ctx = mjs_context_new();
    
    // Execute JavaScript code
    mjs_result_t result = mjs_eval(ctx, "console.log('Hello, World!');");
    
    // Clean up
    mjs_context_free(ctx);
    return 0;
}
```

## License

This project is licensed under the MIT License. See individual source files for copyright information.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests for new functionality
5. Ensure all tests pass
6. Submit a pull request

All contributions must pass the CI/CD pipeline before being merged.