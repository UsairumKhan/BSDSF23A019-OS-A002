CC = gcc
CFLAGS = -Wall

SRC = src/ls.c
OBJ = obj/ls.o
BIN = bin/ls

# Default target
all: $(BIN)

# Link object file to make the final executable
$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $(BIN) $(OBJ)

# Compile the .c file into an object file
$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $(SRC) -o $(OBJ)

# Clean build files
clean:
	rm -f $(OBJ) $(BIN)
