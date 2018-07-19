BIN = hsp-server
OBJS = main.o

.PHONY: all
all: $(BIN)

.PHONY: clean
clean:
	rm -f $(BIN) $(OBJS)

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^
