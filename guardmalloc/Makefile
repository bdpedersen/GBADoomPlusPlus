CXX := clang++
CXXFLAGS := -std=c++17 -Wall -Wextra -g
LDFLAGS := 

# Build directory
BUILD_DIR := build
TEST_DIR := test

# Source files
GUARDMALLOC_SRC := guardmalloc.cc
CORRECTUSE_SRC := $(TEST_DIR)/gmalloc_correctuse.cc
USEAFTERFREE_SRC := $(TEST_DIR)/gmalloc_useafterfree.cc
BUFFEROVERFLOW_SRC := $(TEST_DIR)/gmalloc_bufferoverflow.cc
BUFFERUNDERFLOW_SRC := $(TEST_DIR)/gmalloc_bufferunderflow.cc
TEST_SRC := $(TEST_DIR)/gmalloc_test.cc

# Object files
GUARDMALLOC_OBJ := $(BUILD_DIR)/guardmalloc.o
CORRECTUSE_OBJ := $(BUILD_DIR)/gmalloc_correctuse.o
USEAFTERFREE_OBJ := $(BUILD_DIR)/gmalloc_useafterfree.o
BUFFEROVERFLOW_OBJ := $(BUILD_DIR)/gmalloc_bufferoverflow.o
BUFFERUNDERFLOW_OBJ := $(BUILD_DIR)/gmalloc_bufferunderflow.o
TEST_OBJ := $(BUILD_DIR)/gmalloc_test.o

# Executables
CORRECTUSE_BIN := $(BUILD_DIR)/gmalloc_correctuse
USEAFTERFREE_BIN := $(BUILD_DIR)/gmalloc_useafterfree
BUFFEROVERFLOW_BIN := $(BUILD_DIR)/gmalloc_bufferoverflow
BUFFERUNDERFLOW_BIN := $(BUILD_DIR)/gmalloc_bufferunderflow
TEST_BIN := $(BUILD_DIR)/gmalloc_test

# Phony targets
.PHONY: all test clean

# Default target
all: $(BUILD_DIR) $(CORRECTUSE_BIN) $(USEAFTERFREE_BIN) $(BUFFEROVERFLOW_BIN) $(BUFFERUNDERFLOW_BIN) $(TEST_BIN)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build guardmalloc object file
$(GUARDMALLOC_OBJ): $(GUARDMALLOC_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Build gmalloc_correctuse executable
$(CORRECTUSE_BIN): $(CORRECTUSE_OBJ) $(GUARDMALLOC_OBJ) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(CORRECTUSE_OBJ) $(GUARDMALLOC_OBJ) -o $@ $(LDFLAGS)

# Build gmalloc_useafterfree executable
$(USEAFTERFREE_BIN): $(USEAFTERFREE_OBJ) $(GUARDMALLOC_OBJ) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(USEAFTERFREE_OBJ) $(GUARDMALLOC_OBJ) -o $@ $(LDFLAGS)

# Build gmalloc_bufferoverflow executable
$(BUFFEROVERFLOW_BIN): $(BUFFEROVERFLOW_OBJ) $(GUARDMALLOC_OBJ) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(BUFFEROVERFLOW_OBJ) $(GUARDMALLOC_OBJ) -o $@ $(LDFLAGS)

# Build gmalloc_bufferunderflow executable
$(BUFFERUNDERFLOW_BIN): $(BUFFERUNDERFLOW_OBJ) $(GUARDMALLOC_OBJ) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(BUFFERUNDERFLOW_OBJ) $(GUARDMALLOC_OBJ) -o $@ $(LDFLAGS)

# Build gmalloc_test executable
$(TEST_BIN): $(TEST_OBJ) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

# Compile source files to object files
$(CORRECTUSE_OBJ): $(CORRECTUSE_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(USEAFTERFREE_OBJ): $(USEAFTERFREE_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUFFEROVERFLOW_OBJ): $(BUFFEROVERFLOW_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUFFERUNDERFLOW_OBJ): $(BUFFERUNDERFLOW_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TEST_OBJ): $(TEST_SRC) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Run tests (depends on building all targets first)
test: all
	./$(TEST_BIN)

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)

# Help target
help:
	@echo "Available targets:"
	@echo "  all   - Build all test programs (default)"
	@echo "  test  - Run the test harness (builds all first)"
	@echo "  clean - Remove all build artifacts"
	@echo "  help  - Display this help message"
