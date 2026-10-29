# Game of Life — Contesto per AI Agents

## Stack
C11 · gcc · Make. UI: ncurses (TUI, default) + GTK3 (GUI). Sanitizers: `-fsanitize=address,undefined` (manuali).

## Struttura
- `gol.h/.c` — `GolBoard` + logica pura (stdlib, string.h, time.h)
- `render.h` — interfaccia astratta `RenderBackend`; `render.c` — backend ncurses
- `gtk_render.c` — backend GTK3; `gtk_stub.c` — stub per build ncurses-only
- `main.c` — entry point, CLI (`-r -c -g -h`), ciclo principale
- `Makefile` — build/clean/run, `GTK=1` per GTK3

## RenderBackend
Vedi `render.h`. Ogni backend implementa tutti i metodi della struct (`init, clear, render, cleanup, read_key, set/get_pause, adjust/get_speed, set_blocking, get/set_size, reset_board, get_board_size, clear_grid, process_events`) + factory `const RenderBackend *<name>_backend(void)`.

## Build
- `make` (ncurses) / `make GTK=1` (GTK3) — zero warning obbligatori
- `_DEFAULT_SOURCE` in `main.c` per `usleep()`; `getch` rinominato `read_key` (macro ncurses)

## Convenzioni
- Controllare ogni `malloc`/`calloc`/`realloc`; free su ogni path di errore
- No `strcpy`/`sprintf`/`gets` → `strncpy`/`snprintf`
- No cast per silenziare warning
- Naming: `gol_*` core, `ncurses_*`/`gtk_*` backend
- Commenti e README in italiano

## Nuovo backend
1. `<name>_render.c` con struct contesto privata
2. Implementare tutti i metodi + factory
3. Collegare in `main.c`, aggiornare `Makefile`

## Note
### ncurses
- `gol_step`: doppio buffer + wrapping toroidale `(i + di + rows) % rows`
- `gol_randomize`: seed casuale alla prima chiamata (static flag)
- `nodelay()` via `set_blocking()`: bloccante in pausa, non-bloccante in esecuzione
- Velocità 50–500ms step 100, barra 5 livelli; `q:esci` nella barra; griglia centrata con offset dinamico

### GTK3
- Cell 12px; Cairo su `GtkDrawingArea`; HiDPI via `gtk_widget_get_scale_factor()`
- Resize: ricalcola rows/cols; board esistente → `reset_board()` + `last_key='R'`
- Click sinistro = toggle cella (solo in pausa); F11 fullscreen
- Menu: File (New, Quit) | Game (Random, Clear, Pause) | View (Speed ±) | Help
- Status bar: `Gen: N | PAUSA | ####- | q:esci`
- Tema scuro: bg #1a1a2e, alive #4CAF50, dead #0f0f23, bordi #16213e
- `ctx->size` = board reale, `ctx->cur_rows/cols` = display; `reset_board` NON aggiorna `ctx->size`; `get_board_size` restituisce le dimensioni display
