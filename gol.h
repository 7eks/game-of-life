#ifndef GOL_H
#define GOL_H

#include <stdint.h>

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
