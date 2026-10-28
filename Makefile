CC      = gcc
CSTD    = c11
CFLAGS  = -std=$(CSTD) -Wall -Wextra -Wpedantic -g

GTK_CFLAGS = $(shell pkg-config --cflags gtk+-3.0 2>/dev/null)
GTK_LIBS   = $(shell pkg-config --libs gtk+-3.0 2>/dev/null)

# Supporta: make GTK=1 per GTK, make per ncurses
ifeq ($(GTK),1)
TARGET = gol-gtk
else
TARGET = gol
endif

.PHONY: all clean run run-gtk gtk

# Default: ncurses backend
all: $(TARGET)

# GTK backend (alias esplicito)
gtk:
	$(MAKE) GTK=1

# --- ncurses build ---
gol: main_nc.o gol.o render.o gtk_stub.o
	$(CC) $(CFLAGS) -o $@ $^ -lncurses

# --- GTK build ---
gol-gtk: main_gtk.o gol.o gtk_render.o
	$(CC) $(CFLAGS) -o $@ $^ $(GTK_LIBS)

# --- Compilation rules ---

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# ncurses-specific
main_nc.o: main.c
	$(CC) $(CFLAGS) -DUSE_NCURSES -c -o $@ $<

render.o: render.c
	$(CC) $(CFLAGS) -DUSE_NCURSES -c -o $@ $<

# GTK-specific
main_gtk.o: main.c
	$(CC) $(CFLAGS) -c -o $@ $<

gtk_render.o: gtk_render.c
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -c -o $@ $<

# --- Target ausiliari ---

run: gol
	./gol

run-gtk: gol-gtk
	./gol-gtk

clean:
	rm -f main_nc.o main_gtk.o gol.o render.o gtk_render.o gtk_stub.o gol gol-gtk
