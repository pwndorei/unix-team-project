CC=gcc
TIME=-DTIME
OBJS= common.o create.o server.o client.o
TARGET=project #main binary
LIB=libproject.a #static library
DEST=./build/
SRC=./src/

all: $(TARGET)

$(TARGET): $(LIB)
	$(CC) $(SRC)project.c -lproject -L./build/ -o $(DEST)$@

$(LIB): $(OBJS)
	ar rcs $(DEST)$@ $(DEST)*

common.o:
	$(CC) $(SRC)common.c -o $(DEST)$@ -c

create.o:
	$(CC) $(SRC)create.c -o $(DEST)$@ -c

server.o: 
	$(CC) $(SRC)server.c -o $(DEST)$@ -c

client.o: 
	$(CC) $(SRC)client.c -o $(DEST)$@ -c

clean:
	rm -f ./build/*
