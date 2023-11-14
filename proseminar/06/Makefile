CC := gcc
CC_FLAGS := -O3 -ggdb -std=gnu11 -Wall -Wextra -pedantic -lm -Wgnu-binary-literal

# List the source files
SRC = nbody.c octree.c

# List of Header files
INCLUDE = octree.h

# Derive the object file names from source files
OBJ = $(SRC:.c=.o)

# The final executable
TARGET = nbody

# Default target
all: $(TARGET)

# Rule to build the executable
$(TARGET): $(SRC)
	$(CC) $(CC_FLAGS) -I$(INCLUDE) -o $@ $^

# Clean up generated files
clean:
	rm -f $(OBJ) $(TARGET)
