CC := gcc
CC_FLAGS := -O3 -ggdb -std=gnu11 -Wall -Wextra -pedantic -lm

# List the source files
SRC = nbody.c

# List of Header files
INCLUDE =

# The final executable
TARGET = nbody

# Default target
all: $(TARGET)

# Rule to build the executable
$(TARGET): $(SRC)
	$(CC) $(CC_FLAGS) -o $@ $^

# Clean up generated files
clean:
	rm -f $(TARGET)
