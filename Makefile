# Compiler and flags
CC = clang
CFLAGS = -Wall -Wextra -Werror -pedantic -g

# Directories
SRC_DIR = .
OBJ_DIR = obj
INC_DIR = .

# Source files
SRCS = $(SRC_DIR)/cpu.c \
       $(SRC_DIR)/board.c \
       $(SRC_DIR)/crystos.c \
       $(SRC_DIR)/debug_tools.c \
       $(SRC_DIR)/main.c \

# Object files (generated from source files)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Target executable
TARGET = emulator

# Include directories
INCLUDES = -I$(INC_DIR)

# Default target to build the project
all: $(TARGET)

# Create target executable by linking object files
$(TARGET): $(OBJ_DIR) $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

# Compile source files into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Create object directory if it doesn't exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Clean up object files and executable
clean:
	rm -rf $(OBJ_DIR) $(TARGET)

# Rebuild the project from scratch
rebuild: clean all
