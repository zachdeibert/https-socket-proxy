.PHONY: all
all: hsp-server hsp-client

.PHONY: clean
clean:
	rm -f hsp-server hsp-client server.o client.o

hsp-server: server.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^

hsp-client: client.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^
