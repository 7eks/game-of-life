# Conway's Game of Life

C11 implementation of Conway's Game of Life with two render backends: **ncurses** (TUI) and **GTK3** (GUI), compiled into a single binary.

## Build

```bash
make          # both backends (default)
make clean    # remove build artifacts
```

**Dependencies:** GCC/Clang, Make, ncurses dev headers, GTK3 dev headers (`pkg-config --cflags gtk+-3.0`).

## Run

```bash
./gol                  # ncurses (TUI)
./gol -g               # GTK3 (GUI)
./gol -r 30 -c 60      # custom rows/cols
./gol -h               # usage
```

| Option  | Description |
|---------|-------------|
| `-r ROWS` | Rows (min 5, default 20) |
| `-c COLS` | Cols (min 5, default 40) |
| `-g`      | GTK3 backend |
| `-h`      | Help |

## Controls

| Key / Action | TUI | GUI |
|---|---|---|
| `Space` | Pause / Resume | Pause / Resume |
| `r` | Randomize (gen → 0) | Randomize (gen → 0) |
| `c` | Clear grid (gen → 0) | Clear grid (gen → 0) |
| `+` / `=` | Speed up | Speed up |
| `-` / `_` | Slow down | Slow down |
| `q` | Quit | Quit |
| `F11` | — | Toggle fullscreen |
| Left click | — | Toggle cell (paused only) |
| Resize window | — | Recreate board (random, gen → 0) |

**Speed:** 50–500 ms per step, 100 ms increments. Displayed as a 5-level bar: `#####` (fastest) → `-----` (slowest).

## Architecture

```
main.c          — event loop, timing, CLI
├─ RenderBackend — abstract interface (17 methods)
│   ├─ render.c  → ncurses TUI
│   └─ gtk_render.c → GTK3 GUI
└─ gol.c / gol.h — pure game logic (stdlib only)
```

**`GolBoard`** (`gol.h`): `rows`, `cols`, `**grid`. Functions: `gol_create()`, `gol_free()`, `gol_randomize()`, `gol_clear()`, `gol_step()`.

**`RenderBackend`** (`render.h`): function-pointer struct with 17 methods — `init`, `clear`, `render`, `cleanup`, `read_key`, `set_pause`/`get_pause`, `adjust_speed`/`get_speed`, `set_blocking`, `process_events`, `get_size`/`set_size`, `reset_board`, `get_board_size`, `clear_grid`, `prompt_int`, `clear_prompt`. Factories: `ncurses_backend()`, `gtk_backend()`.

### Core logic (`gol.c`)
- **Double-buffered step** with toroidal wrapping: `(i + di + rows) % rows`
- **Randomize**: 30 % alive, seeded once via `srand(time(NULL))`
- **Step timing**: `nanosleep()` with speed from backend (50–500 ms)
- **Key handling**: non-blocking via `nodelay()` when running, blocking when paused

### GTK3 details
- 12 px cells with Cairo on `GtkDrawingArea`; no HiDPI handling (fixed pixel coordinates)
- Dark theme: bg `#1a1a2e`, alive `#4CAF50`, dead `#0f0f23`, borders `#16213e`
- Menu bar: File · Game · View · Help
- `ctx->size` = real board size; `ctx->cur_rows/cols` = display size
- Resize recalculates rows/cols from `GtkDrawingArea`; triggers `reset_board()` + `last_key = 'R'`

## Rules

Each cell is **alive** or **dead**. At each generation:

- Alive with **< 2** alive neighbours → dies (loneliness)
- Alive with **2–3** alive neighbours → survives
- Alive with **> 3** alive neighbours → dies (overcrowding)
- Dead with **exactly 3** alive neighbours → becomes alive (reproduction)

The grid is **toroidal** — edges wrap around, so patterns travel indefinitely.

## License

MIT
