# Makefile for TinyShell Phase 2
# Modular version with separate source files

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude
LDFLAGS =

# Directories
SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
BIN_DIR = .

# Target executable
TARGET = $(BIN_DIR)/tinyshell

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.c)

# Object files (same names as source files but in obj/ with .o extension)
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOURCES))

# Header files
HEADERS = $(wildcard $(INC_DIR)/*.h)

# Default target
all: $(TARGET)

# Create obj directory if it doesn't exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Link object files to create executable
$(TARGET): $(OBJ_DIR) $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(TARGET)
	@echo "Build complete: $(TARGET)"

# Compile source files to object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(OBJ_DIR)
	rm -f $(TARGET)
	@echo "Clean complete"

# Clean and rebuild
rebuild: clean all

# Show variables (for debugging makefile)
show:
	@echo "Sources: $(SOURCES)"
	@echo "Objects: $(OBJECTS)"
	@echo "Headers: $(HEADERS)"
	@echo "Target: $(TARGET)"

# Run the shell
run: $(TARGET)
	./$(TARGET)

# Phony targets
.PHONY: all clean rebuild show run
