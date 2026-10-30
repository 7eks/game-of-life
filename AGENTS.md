# Game of Life — Context for AI Agents

## Stack
C11 · gcc · Make. UI: ncurses (TUI, default) + GTK3 (GUI) in a single binary (`gol`), GTK selected with `-g`.

## Layout
- `gol.h/.c` — `GolBoard` + pure logic (stdlib/string.h/time.h only, no I/O, no UI)
- `render.h` — abstract `RenderBackend` interface; `render.c` — ncurses backend
- `gtk_render.c` — GTK3 backend
- `main.c` — entry point, CLI (`-r -c -g -h`), main loop
- `Makefile` — build/clean/run, compiles both backends into one binary

## MANDATORY workflow
For every change, in this order:
1. **Read first** all files involved (not just the function you are changing). If you touch `render.h`, read both backends and `main.c`.
2. **Short plan**: write 2–5 lines saying what you will change, why, and which other parts of the code are affected.
3. **Minimal change**: change only what is needed, match the style of the file.
4. **Debug build** (must produce zero warnings):
   ```
   make clean && make CFLAGS="-std=c11 -Wall -Wextra -Wpedantic -g -fsanitize=address,undefined"
   ```
5. **Tests**: run the tests (see below) and report the real output, not an invented summary.
6. **Manual check** of the edge cases in the checklist below for the area you touched.
7. **Update the docs** (this file, `README.md`, `READMEit.md`) if behaviour, commands or structure change. Docs must describe the code as it is, not as it should be.
8. Small commit whose message says what and why.

Never say "done" or "fixed" without having built and tested. If something cannot be verified (e.g. GUI without a display), say so explicitly.

## Tests
- The logic in `gol.c` must have automated `assert` tests (e.g. `test_gol.c` + a `make test` target). If they are missing, create them before modifying `gol.c`.
- Minimum cases: blinker (period 2), block (still life), a glider crossing the toroidal edge and returning to its start position, `gol_create` with size ≤ 0 → NULL, `gol_clear`.
- Every bug fixed in `gol.c` or in CLI parsing → add a test that reproduces it.
- The main loop must never get stuck with no way out: also test with non-terminal stdin (e.g. `./gol </dev/null`) — it must exit with an error, not loop forever.

## Design rules
- **Fix the cause, not the symptom.** Before adding a flag, a callback or a special case, ask whether the problem comes from the architecture. If a fix needs a workaround (fake key codes, pointers nulled "just in case", no-op callbacks), stop and propose the structural fix.
- **Respect the abstraction**: `main.c` must not call ncurses or GTK directly. All I/O goes through `RenderBackend`.
- **Do not grow `RenderBackend` without reason**: a new method must be meaningful for both backends. If a method is a no-op in one backend, the interface is probably wrong.
- **GTK is event-driven**: GTK must own its main loop. No `sleep`/`nanosleep` blocking the GTK thread, no busy loop. Timing is done with `g_timeout_add` (or equivalent), input through signals.
- **No busy-waiting in any backend**: while paused the program must use ~0% CPU.
- **Lifecycle**: every way of quitting (`q` key, menu, closing the window with the X button, Ctrl+C, EOF/input error) must end the process cleanly, freeing everything, with no access to destroyed widgets.
- **Input**: no user event may be lost (no single overwritable "last key" slot).
- **Shared constants** (speed min/max/step, minimum/default sizes) live in one place, not duplicated across backends.

## Edge-case checklist (check the ones for the area you touched)
- CLI: `-r abc`, `-r 10x`, `-r -5`, `-r 99999999999`, option without argument, unknown option, `-h`. Invalid input → clear error and non-zero exit, never a silent default.
- `-r/-c` must have an effect in **both** backends.
- Speed: at the min and max limits press `+`/`-` again; the value must never leave the range and the speed bar must stay consistent.
- ncurses: terminal too small, terminal resized while running, non-terminal stdin.
- GTK: closing the window with the X button, repeated resizes, F11 on and off again, clicks outside the grid, menus used both paused and running, CPU usage while paused (check with `top`).
- Memory: no leaks or errors reported by ASan on exit.

## Conventions
- Check every `malloc`/`calloc`/`realloc`/`fopen`; free on every error path
- No `strcpy`/`sprintf`/`gets` → `snprintf` / bounded versions
- Parse numbers with `strtol` + check `endptr` (empty string or trailing characters = error) + `errno` + range
- No casts or pragmas to silence warnings: fix the code
- Naming: `gol_*` core, `ncurses_*`/`gtk_*` backends
- Language: code comments and user-facing strings (UI, error messages) in Italian; `README.md` in English, `READMEit.md` in Italian — keep the two READMEs in sync

## Adding a backend
1. `<name>_render.c` with a private context struct
2. Implement every method + factory `const RenderBackend *<name>_backend(void)`
3. Wire it into `main.c`, add a compile rule to the `Makefile`
4. Follow the design rules above (no busy loop, clean exit from every path)

## Notes on the current state
### Core
- `gol_step`: double buffer + toroidal wrapping `(i + di + rows) % rows`
- `gol_randomize`: random seed on first call (static flag), 30% density
- Shared constants in `gol.h`: `SPEED_MIN/MAX/STEP/DEFAULT`, `GRID_MIN_ROWS/COLS`, `GRID_DEFAULT_ROWS/COLS`, `CELL_SIZE/PADDING`

### ncurses
- `nodelay()` via `set_blocking()`: blocking while paused, non-blocking while running
- Speed 50–500 ms, step 100, 5-level bar; `q:esci` in the bar; grid centred
- `prompt_int` / `clear_prompt`: ncurses interactive prompts for rows/cols (GTK returns default)

### GTK3
- Cell 12px + 1px padding; Cairo on a `GtkDrawingArea` (no HiDPI handling, fixed pixel coords)
- Resize: recompute rows/cols; existing board → `reset_board()` + `last_key='R'`
- Left click = toggle cell (paused only); F11 = fullscreen
- Menu: File (Esci) | Game (Randomizza, Pulisci, Pausa/Ripresa) | View (Velocità ±) | Help (Scorciatoie)
- Status bar: `Gen: N | PAUSA/IN CORSA | ####- | q:esci`
- Dark theme: bg #1a1a2e, alive #4CAF50, dead #0f0f23, borders #16213e
- `ctx->size` = real board size, `ctx->cur_rows/cols` = display size; `reset_board` does NOT update `ctx->size`; `get_board_size` returns the display size
- RenderBackend has 17 methods (added `prompt_int` and `clear_prompt` for interactive input)
