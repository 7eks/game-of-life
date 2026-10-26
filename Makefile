CC      = gcc
CSTD    = c11
CFLAGS  = -std=$(CSTD) -Wall -Wextra -Wpedantic -g
LDFLAGS = -lncurses

SRCS    = main.c gol.c render.c
OBJS    = $(SRCS:.c=.o)
TARGET  = gol

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
