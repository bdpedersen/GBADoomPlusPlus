# Makefile for TagHeap Test Suite

# Compiler settings
CXX := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -g -O2
CPPFLAGS := -I..

# Directories
BUILD_DIR := build
SRC_DIR := .
PARENT_DIR := ..

# Source files
TEST_SRC := $(SRC_DIR)/test_tagheap.cc
TAGHEAP_SRC := $(PARENT_DIR)/tagheap.cc
SOURCES := $(TEST_SRC) $(TAGHEAP_SRC)

# Object files
OBJECTS := $(BUILD_DIR)/test_tagheap.o $(BUILD_DIR)/tagheap.o

# Output executable
TARGET := $(BUILD_DIR)/test_tagheap

# Default target
.PHONY: all
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJECTS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $^
	@echo "Build successful: $(TARGET)"

# Compile test source
$(BUILD_DIR)/test_tagheap.o: $(TEST_SRC)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

# Compile tagheap source
$(BUILD_DIR)/tagheap.o: $(TAGHEAP_SRC)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

# Run the tests
.PHONY: run
run: $(TARGET)
	@$(TARGET)

# Run with verbose output
.PHONY: run-verbose
run-verbose: $(TARGET)
	@$(TARGET) 2>&1 | tee $(BUILD_DIR)/test_results.txt

# Clean build artifacts
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	@echo "Cleaned build artifacts"

# Rebuild from scratch
.PHONY: rebuild
rebuild: clean all

# Display help
.PHONY: help
help:
	@echo "TagHeap Test Suite - Available targets:"
	@echo ""
	@echo "  all              - Build the test executable (default)"
	@echo "  run              - Build and run tests"
	@echo "  run-verbose      - Run tests with verbose output and save results"
	@echo "  clean            - Remove build artifacts"
	@echo "  rebuild          - Clean and build from scratch"
	@echo "  help             - Display this help message"
	@echo ""

# PHONY targets
.PHONY: all run run-verbose clean rebuild help
