SRC = cmus2tsv.c
OBJ = $(SRC:.c=.o)
BIN = cmus2tsv
CFLAGS = -g3 -O0

all: $(BIN)
$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(BIN) $(OBJ)

clean:
	rm $(BIN) $(OBJ)

.PHONY: all clean
