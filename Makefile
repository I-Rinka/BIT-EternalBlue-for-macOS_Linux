CC:=gcc
CFLAGS:=-shared -fPIC -Wall

all: payload.so 

payload.so: payload.c config.h
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *payload.so
