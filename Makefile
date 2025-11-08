# Makefile for Linux Windows Registry API

CC = gcc
AR = ar
CFLAGS = -Wall -Wextra -O2 -fPIC -Iinclude
LDFLAGS = -shared
LIBS = -lsqlite3 -lpthread

# Directories
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
EXAMPLES_DIR = examples

# Library name
LIB_NAME = libwinreg
STATIC_LIB = $(BUILD_DIR)/$(LIB_NAME).a
SHARED_LIB = $(BUILD_DIR)/$(LIB_NAME).so

# Source files
SOURCES = $(SRC_DIR)/winreg.c \
          $(SRC_DIR)/db_backend.c \
          $(SRC_DIR)/registry_utils.c

# Object files
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Example programs
EXAMPLES = $(BUILD_DIR)/test_registry

# Default target
all: directories $(STATIC_LIB) $(SHARED_LIB) $(EXAMPLES)

# Create build directory
directories:
	@mkdir -p $(BUILD_DIR)

# Compile object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Build static library
$(STATIC_LIB): $(OBJECTS)
	$(AR) rcs $@ $^
	@echo "Static library built: $@"

# Build shared library
$(SHARED_LIB): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	@echo "Shared library built: $@"

# Build examples
$(BUILD_DIR)/test_registry: $(EXAMPLES_DIR)/test_registry.c $(STATIC_LIB)
	$(CC) $(CFLAGS) -o $@ $< -L$(BUILD_DIR) -lwinreg $(LIBS)
	@echo "Example program built: $@"

# Install target
install: all
	@echo "Installing headers..."
	@mkdir -p /usr/local/include/winreg
	@cp -f $(INC_DIR)/*.h /usr/local/include/winreg/
	@echo "Installing libraries..."
	@cp -f $(STATIC_LIB) /usr/local/lib/
	@cp -f $(SHARED_LIB) /usr/local/lib/
	@ldconfig
	@echo "Installation complete!"

# Uninstall target
uninstall:
	@echo "Uninstalling..."
	@rm -rf /usr/local/include/winreg
	@rm -f /usr/local/lib/$(LIB_NAME).*
	@ldconfig
	@echo "Uninstall complete!"

# Run tests
test: $(BUILD_DIR)/test_registry
	@echo "Running tests..."
	@$(BUILD_DIR)/test_registry

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)
	@echo "Clean complete!"

# Help target
help:
	@echo "Available targets:"
	@echo "  all        - Build libraries and examples (default)"
	@echo "  install    - Install headers and libraries to /usr/local"
	@echo "  uninstall  - Remove installed files"
	@echo "  test       - Build and run test programs"
	@echo "  clean      - Remove build artifacts"
	@echo "  help       - Show this help message"

.PHONY: all directories install uninstall test clean help
