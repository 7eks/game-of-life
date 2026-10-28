# Game of Life — Conway

Implementazione in C del **Conway's Game of Life** con due interfacce: **TUI** (ncurses) e **GUI** (GTK3).

## Requisiti

### Dipendenze comuni
- GCC (o clang)
- Make

### TUI (ncurses)
```bash
# Su Arch Linux / Omarchy
sudo pacman -S ncurses

# Su Debian / Ubuntu
sudo apt install libncurses5-dev
```

### GUI (GTK3)
```bash
# Su Arch Linux / Omarchy
sudo pacman -S gtk3

# Su Debian / Ubuntu
sudo apt install libgtk-3-dev
```

## Build

### Backend ncurses (default)
```bash
make
```

### Backend GTK3
```bash
make GTK=1
```

Per pulire i file compilati:
```bash
make clean
```

## Esecuzione

### Backend ncurses (default)
```bash
make run
# oppure
./gol
```

### Backend GTK3
```bash
./gol -g
```

È possibile specificare le dimensioni della griglia direttamente da riga di comando:

```bash
./gol -r 30 -c 60   # 30 righe, 60 colonne
./gol -r 10        # 10 righe, 40 colonne (default)
./gol -c 80        # 20 righe (default), 80 colonne
./gol -h           # Mostra aiuto
```

### Opzioni CLI

| Opzione | Descrizione |
|---------|------------|
| `-r ROWS` | Numero di righe (min: 5, default: 20) |
| `-c COLS` | Numero di colonne (min: 5, default: 40) |
| `-g` | Usa backend GTK3 (GUI) invece di ncurses |
| `-h` | Mostra aiuto |

Se le dimensioni non vengono fornite via CLI, viene richiesto un prompt interattivo (solo ncurses).

## Utilizzo

La griglia viene randomizzata automaticamente e la simulazione parte.

### Controlli TUI (ncurses)

| Tasto | Azione |
|-------|--------|
| `Spazio` | Pausa / Ripresa |
| `r` | Randomizza la griglia (reset generazione) |
| `c` | Pulisci la griglia (reset generazione) |
| `+` / `=` | Aumenta velocità (più veloce) |
| `-` / `_` | Diminuisci velocità (più lento) |
| `q` | Esci |

La velocità è visualizzata come barra a 5 livelli: `#####` (50ms, max) → `-----` (500ms, min).
`+` e `-` regolano la velocità in step da 100ms.

### Controlli GUI (GTK3)

| Tasto / Azione | Descrizione |
|----------------|------------|
| `F11` | Toggle fullscreen |
| `Spazio` | Pausa / Ripresa |
| `r` | Randomizza la griglia (reset generazione) |
| `c` | Pulisci la griglia (reset generazione) |
| `+` / `=` | Aumenta velocità (più veloce) |
| `-` / `_` | Diminuisci velocità (più lento) |
| `q` | Esci |
| `Click sinistro` | Toggle cella (solo in pausa) |

La velocità è visualizzata nella status bar in basso con barra a 5 livelli: `#####` (50ms, max) → `-----` (500ms, min).

## Architettura

Il progetto è strutturato in tre layer separati:

- **Core** (`gol.h` / `gol.c`) — logica pura del gioco, zero dipendenze esterne.
  Gestisce allocazione, step, randomizzazione e pulizia della griglia.

- **Render** (`render.h`) — interfaccia astratta tramite `RenderBackend`,
  una struct di function pointer che ogni backend deve implementare.

- **Backend** (`render.c`, `gtk_render.c`) — implementazioni concrete:
  - `render.c` → backend ncurses (TUI, terminale)
  - `gtk_render.c` → backend GTK3 (GUI, windowing nativo)

```
┌─────────────────────────────────────────┐
│              main.c                     │
│   (ciclo principale, input, timing)     │
├─────────────────────────────────────────┤
│   RenderBackend (interfaccia astratta)   │
│   init / clear / render / cleanup       │
│   read_key / set_pause / get_pause      │
│   adjust_speed / get_speed              │
│   set_blocking / get_size / set_size    │
├─────────────────────────────────────────┤
│   render.c              → ncurses TUI   │
│   gtk_render.c          → GTK3 GUI      │
├─────────────────────────────────────────┤
│   gol.c / gol.h                         │
│   (logica pura del gioco)               │
│   board toroidale con wrapping          │
└─────────────────────────────────────────┘
```

Per aggiungere un nuovo backend (es. SDL, WebGL) basta implementare
l'interfaccia `RenderBackend` in un nuovo file senza modificare `gol.c`.

## Regole del Game of Life

Ogni cella può essere **viva** o **morta**. Ad ogni generazione:

- Una cella viva con **meno di 2** vicini vivi muore (solitudine)
- Una cella viva con **2 o 3** vicini vivi sopravvive
- Una cella viva con **più di 3** vicini vivi muore (sovraffollamento)
- Una cella morta con **esattamente 3** vicini vivi diventa viva (riproduzione)

### Bordi avvolgenti

La griglia è **toroidale**: i bordi si avvolgono su se stessi.
Un pattern che esce da un lato rientra dal lato opposto (come su un nastro di Moebius).
Questo permette ai glider e ad altri pattern di viaggiare indefinitamente.

## Licenza

MIT
