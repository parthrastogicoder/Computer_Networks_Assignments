# Compiler
CC = gcc

# Compiler flags
CFLAGS = -pthread

# Source files
SRCS = q1_server.c multi_thread_server.c single_thread_server.c select_server.c client.c

# Output executables
EXECS = $(SRCS:.c=)

# Default target
all: $(EXECS)

# Rule to compile each source file into an executable
%: %.c
	$(CC) $(CFLAGS) -o $@ $<

# Clean target to remove executables
clean:
	rm -f $(EXECS)

# Phony targets
.PHONY: all clean
