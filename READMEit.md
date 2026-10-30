# Il Gioco della Vita di Conway

Implementazione in C11 del **Gioco della Vita di Conway** con due backend di rendering: **ncurses** (TUI) e **GTK3** (GUI), compilati in un unico binario.

## Build

```bash
make          # entrambi i backend (default)
make clean    # rimuove i file di build
```

**Dipendenze:** GCC/Clang, Make, header di sviluppo ncurses, header di sviluppo GTK3 (`pkg-config --cflags gtk+-3.0`).

## Esecuzione

```bash
./gol                  # ncurses (TUI)
./gol -g               # GTK3 (GUI)
./gol -r 30 -c 60      # righe/colonne personalizzate
./gol -h               # aiuto
```

| Opzione   | Descrizione |
|-----------|-----------|
| `-r RIGHE` | Numero di righe (min 5, default 20) |
| `-c COL`   | Numero di colonne (min 5, default 40) |
| `-g`       | Backend GTK3 |
| `-h`       | Aiuto |

## Controlli

| Tasto / Azione | TUI | GUI |
|---|---|---|
| `Spazio` | Pausa / Ripresa | Pausa / Ripresa |
| `r` | Randomizza (gen → 0) | Randomizza (gen → 0) |
| `c` | Pulisci griglia (gen → 0) | Pulisci griglia (gen → 0) |
| `+` / `=` | Aumenta velocità | Aumenta velocità |
| `-` / `_` | Diminuisci velocità | Diminuisci velocità |
| `q` | Esci | Esci |
| `F11` | — | Toggle fullscreen |
| Click sinistro | — | Toggle cella (solo in pausa) |
| Ridimensiona finestra | — | Ricrea board (random, gen → 0) |

**Velocità:** 50–500 ms per step, incrementi di 100 ms. Visualizzata come barra a 5 livelli: `#####` (massima) → `-----` (minima).

## Architettura

```
main.c          — ciclo eventi, temporizzazione, CLI
├─ RenderBackend — interfaccia astratta (15 metodi)
│   ├─ render.c  → ncurses TUI
│   └─ gtk_render.c → GTK3 GUI
└─ gol.c / gol.h — logica pura del gioco (solo stdlib)
```

**`GolBoard`** (`gol.h`): `rows`, `cols`, `**grid`. Funzioni: `gol_create()`, `gol_free()`, `gol_randomize()`, `gol_clear()`, `gol_step()`.

**`RenderBackend`** (`render.h`): struct di function pointer con 15 metodi — `init`, `clear`, `render`, `cleanup`, `read_key`, `set_pause`/`get_pause`, `adjust_speed`/`get_speed`, `set_blocking`, `process_events`, `get_size`/`set_size`, `reset_board`, `get_board_size`, `clear_grid`. Factory: `ncurses_backend()`, `gtk_backend()`.

### Logica core (`gol.c`)
- **Step a doppio buffer** con wrapping toroidale: `(i + di + rows) % rows`
- **Randomizzazione**: 30 % vive, seedata una volta con `srand(time(NULL))`
- **Tempismo step**: `nanosleep()` con velocità dal backend (50–500 ms)
- **Gestione input**: non bloccante via `nodelay()` in esecuzione, bloccante in pausa

### Dettagli GTK3
- Celle da 12 px con Cairo su `GtkDrawingArea`; HiDPI tramite `scale_factor()`
- Tema scuro: sfondo `#1a1a2e`, vive `#4CAF50`, morte `#0f0f23`, bordi `#16213e`
- Barra dei menu: File · Gioco · Visualizza · Aiuto
- `ctx->size` = dimensione reale board; `ctx->cur_rows/cols` = dimensioni display
- Il ridimensionamento ricalcola righe/colonne dalla `GtkDrawingArea`; attiva `reset_board()` + `last_key = 'R'`

## Regole

Ogni cella è **viva** o **morta**. Ad ogni generazione:

- Viva con **< 2** vicine vive → muore (solitudine)
- Viva con **2–3** vicine vive → sopravvive
- Viva con **> 3** vicine vive → muore (sovraffollamento)
- Morta con **esattamente 3** vicine vive → diventa viva (riproduzione)

La griglia è **toroidale** — i bordi si avvolgono, quindi i pattern viaggiano indefinitamente.

## Licenza

MIT
