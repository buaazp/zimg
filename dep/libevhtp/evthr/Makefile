SRC      = evthr.c
OUT      = libevthr.a
OBJ      = $(SRC:.c=.o)
INCLUDES = -I.
CFLAGS   += -Wall -Wextra -ggdb
LDFLAGS  += -ggdb
CC       = gcc

.SUFFIXES: .c

default: $(OUT)

.c.o:
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	ar rcs $(OUT) $(OBJ)

test: $(OUT) test.c
	$(CC) $(INCLUDES) $(CFLAGS) test.c -o test $(OUT) -levent -levent_pthreads -lpthread

clean:
	rm -f $(OBJ) $(OUT) test

