# Paths
SRC_DIR := src
BIN_DIR := bin
BUILD_DIR := build

# Files
TARGET := $(BIN_DIR)/server
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Compiler
CC := gcc
CFLAGS := -Wall -Wextra -g
ifdef CONSOLE
CFLAGS += -DLOGGER_ENABLE_CONSOLE
endif

# Default target
all: $(TARGET)

# Link the binary
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(OBJS) -o $@

# Compile .c to .o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create output directories if needed
$(BIN_DIR) $(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

run: $(TARGET)
	./$(TARGET)

