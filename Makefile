# MikoJSCore Makefile for WSL/Linux
# Converted from CMakeLists.txt

# Project configuration
PROJECT_NAME = MikoJSCore
VERSION = 0.1.0

# Compiler settings
CC = wsl gcc
CXX = wsl g++
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2 -Wno-unused-parameter
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O2
LDFLAGS = -lm

# Directories
SRC_DIR = src
INCLUDE_DIR = include
SHELL_DIR = shell
# TEST_DIR = tests
BUILD_DIR = build_linux
LIB_DIR = $(BUILD_DIR)/lib
BIN_DIR = $(BUILD_DIR)/bin
OBJ_DIR = $(BUILD_DIR)/obj

# Include paths
INCLUDES = -I$(INCLUDE_DIR) -I$(SRC_DIR)

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Shell source
SHELL_SRC = $(SHELL_DIR)/shell.c
SHELL_OBJ = $(OBJ_DIR)/shell.o

# Test sources (commented out as per request)
# TEST_SOURCES = $(wildcard $(TEST_DIR)/*.c)
# TEST_OBJECTS = $(TEST_SOURCES:$(TEST_DIR)/%.c=$(OBJ_DIR)/test_%.o)

# Library and executable names
LIBRARY = $(LIB_DIR)/libmikojs.a
EXECUTABLE = $(BIN_DIR)/miko

# Default target
all: directories $(LIBRARY) $(EXECUTABLE)

# Create necessary directories
directories:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(LIB_DIR)
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(OBJ_DIR)

# Compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile shell source
$(SHELL_OBJ): $(SHELL_SRC)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Create static library
$(LIBRARY): $(OBJECTS)
	@echo "Creating static library: $@"
	ar rcs $@ $^

# Create executable
$(EXECUTABLE): $(SHELL_OBJ) $(LIBRARY)
	@echo "Creating executable: $@"
	$(CC) $< -L$(LIB_DIR) -lmikojs $(LDFLAGS) -o $@

# Test targets (commented out)
# $(OBJ_DIR)/test_%.o: $(TEST_DIR)/%.c
# 	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@
# 
# test: directories $(LIBRARY) $(TEST_OBJECTS)
# 	@echo "Building tests..."
# 	$(CC) $(LDFLAGS) $(TEST_OBJECTS) -L$(LIB_DIR) -lmikojs -o $(BIN_DIR)/mikojs_test

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILD_DIR)

# Install targets
install: all
	@echo "Installing MikoJS..."
	mkdir -p /usr/local/lib
	mkdir -p /usr/local/bin
	mkdir -p /usr/local/include
	cp $(LIBRARY) /usr/local/lib/
	cp $(EXECUTABLE) /usr/local/bin/
	cp -r $(INCLUDE_DIR)/* /usr/local/include/

# Uninstall
uninstall:
	@echo "Uninstalling MikoJS..."
	rm -f /usr/local/lib/libmikojs.a
	rm -f /usr/local/bin/miko
	rm -rf /usr/local/include/mikojs.h

# Show help
help:
	@echo "MikoJSCore Makefile"
	@echo "Available targets:"
	@echo "  all        - Build library and executable (default)"
	@echo "  clean      - Remove build artifacts"
	@echo "  install    - Install to system directories"
	@echo "  uninstall  - Remove from system directories"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Build outputs:"
	@echo "  Library:    $(LIBRARY)"
	@echo "  Executable: $(EXECUTABLE)"

# Debug target to show variables
debug:
	@echo "Sources: $(SOURCES)"
	@echo "Objects: $(OBJECTS)"
	@echo "Library: $(LIBRARY)"
	@echo "Executable: $(EXECUTABLE)"

# Phony targets
.PHONY: all clean install uninstall help debug directories

# Dependency tracking
-include $(OBJECTS:.o=.d)
-include $(SHELL_OBJ:.o=.d)

# Generate dependency files
$(OBJ_DIR)/%.d: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -MM -MT $(@:.d=.o) $< > $@

$(OBJ_DIR)/shell.d: $(SHELL_SRC)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDES) -MM -MT $(SHELL_OBJ) $< > $@