# Default compiler prefix (empty for native compilation)
CROSS_COMPILE ?=

CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Werror
TARGET = writer
OBJS = writer.o
all: $(TARGET)

# Link object files into the final executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Compile C source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target to remove the executable and object files
clean:
	rm -f $(TARGET) $(OBJS)

# Declare phony targets to prevent conflicts with files of the same name
.PHONY: all clean
