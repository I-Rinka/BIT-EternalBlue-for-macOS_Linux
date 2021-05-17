CC:=gcc
CFLAGS:=-shared -fPIC -Wall -Wno-nonnull

all: payload.so 

payload.so: payload.c config.h
	$(CC) $(CFLAGS) $< -o $@
# payload.so: payload.c config.h
# 	$(CC) $(CFLAGS) implant.c -o $@

clean:
	rm -f *payload.so
