.PHONY: all
all: hsp-server

.PHONY: clean
clean:
	rm -f hsp-server server.o

hsp-server: server.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^
