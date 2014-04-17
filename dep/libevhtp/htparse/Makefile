SRC      = htparse.c
OUT      = libhtparse.a
OBJ      = $(SRC:.c=.o)
INCLUDES = -I.
CFLAGS   += -ggdb
LDFLAGS  +=
CC       = gcc

.SUFFIXES: .c

default: $(OUT) test

.c.o:
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	ar rcs $(OUT) $(OBJ)

test: $(OUT) test.c
	$(CC) $(INCLUDES) $(CFLAGS) test.c -o test $(OUT)

clean:
	rm -f $(OBJ) $(OUT) test
