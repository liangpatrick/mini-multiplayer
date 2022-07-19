OUTPUT = server
CFLAGS = -g -Wall -Wvla
LDFLAGS = -pthread

%.o: %.c %.h
		gcc $(CFLAGS) -c -o $@ $<
%.o: %.c
		gcc $(CFLAGS) -c -o $@ $<

all: $(OUTPUT)

server: server.o 
		gcc $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
		rm -f $(OUTPUT) *.o