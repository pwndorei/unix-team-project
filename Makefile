CC=gcc
CFLAGS=-g -DTIMES -std=gnu99
OBJS= common.o create.o server.o client.o
TARGET=project
LIB=libproject.a
DEST=./build/
SRC=./src/

all: $(DEST)$(TARGET)

$(DEST)$(TARGET): $(LIB) $(DEST)project.o
	$(CC) $(DEST)project.o -lproject -L$(DEST) -o $@ $(CFLAGS)

$(LIB): $(addprefix $(DEST), $(OBJS))
	ar rcs $(DEST)$@ $(addprefix $(DEST), $(OBJS))

$(DEST)%.o: $(SRC)%.c
	$(CC) $< -o $@ -c $(CFLAGS)

clean:
	rm -f $(DEST)*
