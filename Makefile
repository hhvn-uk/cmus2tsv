PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
SRC = cmus2tsv.c
OBJ = $(SRC:.c=.o)
BIN = cmus2tsv
CFLAGS = -g3 -O0
SH = cmusextractpl

all: $(BIN)
$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(BIN) $(OBJ)

clean:
	rm $(BIN) $(OBJ)

install:
	install -m0755 $(BIN) $(BINDIR)/$(BIN)
	install -m0755 $(SH) $(BINDIR)/$(SH)

uninstall:
	rm -f $(BINDIR)/$(BIN)

.PHONY: all clean install uninstall
