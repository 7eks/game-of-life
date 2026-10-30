#ifndef GOL_H
#define GOL_H

#include <stdint.h>

/* --- Costanti condivise --- */
#define SPEED_MIN         50
#define SPEED_MAX         500
#define SPEED_STEP        100
#define SPEED_DEFAULT     250

#define GRID_MIN_ROWS     5
#define GRID_MIN_COLS     5
#define GRID_DEFAULT_ROWS 20
#define GRID_DEFAULT_COLS 40

#define CELL_SIZE         12
#define CELL_PADDING      1

typedef struct {
    int    rows;
    int    cols;
    int   **grid;
} GolBoard;

GolBoard *gol_create(int rows, int cols);
void      gol_free(GolBoard *b);
void      gol_randomize(GolBoard *b);
void      gol_clear(GolBoard *b);
void      gol_step(GolBoard *b);

#endif /* GOL_H */
