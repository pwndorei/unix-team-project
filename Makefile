CC=gcc
CFLAGS=-g
TIME=-DTIME
OBJS= server.o client.o
TARGET=project #main binary
LIB=-libprject.a #static library
DEST=./build/
SRC=./src/

all: $(TARGET)

create:
	$(CC) $(SRC)create.c -o $(DEST)$@

server.o: 
	$(CC) $(SRC)server.c -o $(DEST)$@ -c

client.o: 
	$(CC) $(SRC)client.c -o $(DEST)$@

clean:
	rm -f ./build/*
