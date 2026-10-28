# Game of Life — Contesto per AI Agents

## Stack tecnologico

- **Linguaggio**: C11
- **Compiler**: gcc (clang disponibile ma non testato)
- **Build system**: Make
- **UI**: ncurses (TUI) + GTK3 (GUI)
- **Sanitizzazione**: AddressSanitizer + UndefinedBehaviorSanitizer (da attivare manualmente con `-fsanitize=address,undefined`)

## Struttura del progetto

```
gol/
├── Makefile        — regola build, target clean/run, flag GTK=1
├── gol.h           — struct GolBoard + prototype funzioni core
├── gol.c           — implementazione logica pura (no dipendenze esterne)
├── render.h        — interfaccia astratta RenderBackend
├── render.c        — implementazione backend ncurses
├── gtk_render.c    — implementazione backend GTK3 (GUI)
├── gtk_stub.c      — stub GTK per build senza dipendenze (ncurses only)
├── main.c          — entry point, parsing CLI (-r, -c, -g, -h), ciclo principale
├── README.md       — documentazione utente
└── AGENTS.md       — questo file
```

## Responsabilità di ciascun file

| File | Responsabilità | Dipendenze esterne |
|---|---|---|
| `gol.h` / `gol.c` | Allocazione, step, random, clear della board | stdlib, string.h, time.h |
| `render.h` | Interfaccia `RenderBackend` (function pointers) | stdint.h |
| `render.c` | Implementazione ncurses del backend (barra velocità 5 livelli, info in 2 righe) | ncurses.h, stdlib.h |
| `gtk_render.c` | Backend GTK3 (Cairo rendering, menu, status bar, mouse, fullscreen, resize) | gtk+-3.0, stdlib.h |
| `gtk_stub.c` | Stub GTK per build ncurses-only (fallback quando GTK non disponibile) | — |
| `main.c` | Parsing opzioni CLI (`-r`, `-c`, `-g`, `-h`), fallback prompt interattivo, ciclo principale, input handling, resize board | gol.h, render.h, stdio.h, stdlib.h, unistd.h |

## Interfaccia RenderBackend (da citare)

```c
typedef struct { int rows; int cols; } RenderSize;

typedef struct RenderBackend RenderBackend;

struct RenderBackend {
    void   *(*init)(RenderSize size);
    void    (*clear)(void *ctx);
    void    (*render)(void *ctx, int **grid, uint64_t gen);
    void    (*cleanup)(void *ctx);
    int     (*read_key)(void *ctx);
    void    (*set_pause)(void *ctx, int paused);
    int     (*get_pause)(void *ctx);
    void    (*adjust_speed)(void *ctx, int delta);
    int     (*get_speed)(void *ctx);
    void    (*set_blocking)(void *ctx, int blocking);
    void    (*get_size)(void *ctx, int *max_rows, int *max_cols);
    void    (*set_size)(void *ctx, RenderSize size);
    void    (*reset_board)(void *ctx);
    void    (*get_board_size)(void *ctx, int *rows, int *cols);
    void    (*clear_grid)(void *ctx);
    void    (*process_events)(void *ctx);
};

const RenderBackend *ncurses_backend(void);
const RenderBackend *gtk_backend(void);
```

Ogni nuovo backend (SDL, GTK, ecc.) deve implementare queste 15 funzioni e
fornire una factory simile a `ncurses_backend()`.

## Regole di build

### ncurses (default)
```bash
gcc -std=c11 -Wall -Wextra -Wpedantic -g -c -o main.o main.c
gcc -std=c11 -Wall -Wextra -Wpedantic -g -o gol main.o gol.o render.o -lncurses
```

### GTK3
```bash
pkg-config --cflags gtk+-3.0   # flags di compilazione
pkg-config --libs gtk+-3.0     # flags di linking
gcc -std=c11 -Wall -Wextra -Wpedantic -g -c -o main.o main.c
gcc -std=c11 -Wall -Wextra -Wpedantic -g -o gol main.o gol.o gtk_render.o $(pkg-config --cflags --libs gtk+-3.0)
```

- **Zero warning** obbligatori
- `_DEFAULT_SOURCE` definito in `main.c` per `usleep()` in C11
- `getch` rinominato in `read_key` per evitare conflitto con macro ncurses

## Convenzioni di codice

- Controllare ogni `malloc`/`calloc`/`realloc`, free su ogni path di errore
- Non usare `strcpy`/`sprintf`/`gets` → preferire `strncpy`/`snprintf`
- No cast per silenziare warning
- Nomi funzioni: `gol_*` per core, `ncurses_*` per backend ncurses, `gtk_*` per backend GTK3, `ncurses_backend()` / `gtk_backend()` per factory interfaccia
- Commenti in italiano nel codice, README in italiano

## Come aggiungere un nuovo backend

1. Creare `sdl_render.c` (o nome simile)
2. Definire struct di contesto privata (es. `SdlCtx`)
3. Implementare le 15 funzioni dell'interfaccia
4. Esportare `const RenderBackend *sdl_backend(void)`
5. Modificare `main.c` per usare il nuovo backend al posto di `ncurses_backend()`
6. Aggiornare `Makefile` per includere il nuovo file sorgente

## Note implementative

### ncurses
- `gol_step` usa doppio buffer (alloca 2 grid, swap pointer)
- `gol_step` usa **wrapping toroidale**: `(i + di + rows) % rows` per i vicini,
  così un glider che esce da un lato rientra dall'opposto
- `gol_randomize` usa seed casuale alla prima chiamata (static seeded flag)
- `RenderCtx` in `render.c` contiene `WINDOW *win` per `set_blocking()`
- ncurses usa `nodelay()` via `set_blocking()`: bloccante in pausa, non-bloccante in esecuzione
- Velocità: scala 50-500ms in step da 100ms, display a barra 5 livelli (`#####` a `-----`)
- `q:esci` visualizzato a destra della barra velocità nella riga info inferiore
- Griglia centrata nel terminale con offset calcolato dinamicamente

### GTK3
- **Cell size**: 12px fissi (coordinate logical)
- **Resize finestra**: `on_configure` ricalcola rows/cols = available / cell_size.
  Se `ctx->grid != NULL` (finestra già creata, board esistente), chiama
  `reset_board()` → imposta `last_key = 'R'` → main loop ricrea board con nuove
  dimensioni + random + gen=0. Se `ctx->grid == NULL` (creazione finestra), ignora.
- **Mouse**: click sinistro = toggle cella (solo in pausa, condizione `!ctx->paused`)
- **Fullscreen**: F11 toggle
- **Menu**: File (New, Quit) | Game (Random, Clear, Pause) | View (Speed +, Speed -) | Help (Shortcuts)
- **Status bar**: riga in basso — `Gen: N | PAUSA | ####- | q:esci`
- **Tema scuro**: sfondo #1a1a2e, alive #4CAF50, dead #0f0f23, bordi #16213e
- **Rendering**: Cairo su `GtkDrawingArea`
- **Input**: GDK events → key code per `read_key()`
- **Scale factor**: GTK3 espone `gtk_widget_get_scale_factor()` per HiDPI, Cairo gestisce automaticamente
- **Dimensioni**: `ctx->size` = dimensioni reali board, `ctx->cur_rows/cols` = dimensioni display.
  `gtk_backend_render` sincronizza display → board (`ctx->cur_rows = ctx->size.rows`).
  `reset_board` NON aggiorna `ctx->size` (resta pari alla board attuale).
  `get_board_size` restituisce `ctx->cur_rows/cols` (window dims) per resize.
