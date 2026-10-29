CC      = gcc
CSTD    = c11
CFLAGS  = -std=$(CSTD) -Wall -Wextra -Wpedantic -g

GTK_CFLAGS = $(shell pkg-config --cflags gtk+-3.0 2>/dev/null)
GTK_LIBS   = $(shell pkg-config --libs gtk+-3.0 2>/dev/null)

TARGET = gol

.PHONY: all clean run gtk

# Default: single binary with both backends
all: $(TARGET)

# Alias esplicito (per compatibilita')
gtk:
	$(MAKE)

# --- Build ---

$(TARGET): main.o gol.o render.o gtk_render.o
	$(CC) $(CFLAGS) -o $@ $^ -lncurses $(GTK_LIBS)

# --- Compilation rules ---

%.o: %.c
	$(CC) $(CFLAGS) -DUSE_NCURSES -c -o $@ $<

gtk_render.o: gtk_render.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -DUSE_NCURSES -c -o $@ $<

# --- Target ausiliari ---

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f main.o gol.o render.o gtk_render.o $(TARGET)
