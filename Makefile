# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
LDFLAGS =

# Target executable (current directory)
TARGET = picodb

# Source directory
SRC_DIR = src

# Include directories
INCLUDES = \
	-I$(SRC_DIR) \
	-I$(SRC_DIR)/commands \
	-I$(SRC_DIR)/index \
	-I$(SRC_DIR)/parser \
	-I$(SRC_DIR)/storage

# Source files
SOURCES = \
	$(SRC_DIR)/main.cpp \
	$(SRC_DIR)/commands/commands.cpp \
	$(SRC_DIR)/commands/create.cpp \
	$(SRC_DIR)/commands/insert.cpp \
	$(SRC_DIR)/commands/select.cpp \
	$(SRC_DIR)/commands/show.cpp \
	$(SRC_DIR)/commands/utils.cpp \
	$(SRC_DIR)/index/bplusTree_index.cpp \
	$(SRC_DIR)/index/hash_index.cpp \
	$(SRC_DIR)/parser/parser.cpp \
	$(SRC_DIR)/storage/bitfield.cpp \
	$(SRC_DIR)/storage/file_manager.cpp \
	$(SRC_DIR)/storage/varint.cpp

# Default target
all:
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SOURCES) -o $(TARGET) $(LDFLAGS)
	@echo "Build successful! Executable: ./$(TARGET)"

# Run
run: all
	./$(TARGET)

# Debug
debug:
	$(CXX) $(CXXFLAGS) -g -DDEBUG $(INCLUDES) $(SOURCES) -o $(TARGET)

# Release
release:
	$(CXX) $(CXXFLAGS) -O3 $(INCLUDES) $(SOURCES) -o $(TARGET)

# Clean
clean:
	rm -f $(TARGET)
	@echo "Clean complete!"

.PHONY: all run debug release clean
