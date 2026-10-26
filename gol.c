#include "gol.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

GolBoard *gol_create(int rows, int cols)
{
    if (rows <= 0 || cols <= 0)
        return NULL;

    GolBoard *b = calloc(1, sizeof(GolBoard));
    if (!b)
        return NULL;

    b->rows = rows;
    b->cols = cols;

    /* alloca array di puntatori */
    b->grid = calloc((size_t)rows, sizeof(int *));
    if (!b->grid) {
        free(b);
        return NULL;
    }

    /* alloca ogni riga */
    for (int i = 0; i < rows; i++) {
        b->grid[i] = calloc((size_t)cols, sizeof(int));
        if (!b->grid[i]) {
            /* rollback sulle righe già allocate */
            for (int j = 0; j < i; j++)
                free(b->grid[j]);
            free(b->grid);
            free(b);
            return NULL;
        }
    }

    return b;
}

void gol_free(GolBoard *b)
{
    if (!b)
        return;

    if (b->grid) {
        for (int i = 0; i < b->rows; i++)
            free(b->grid[i]);
        free(b->grid);
    }
    free(b);
}

void gol_randomize(GolBoard *b)
{
    if (!b)
        return;

    /* usa seed casuale solo alla prima chiamata */
    static int seeded = 0;
    if (!seeded) {
        srand((unsigned)time(NULL));
        seeded = 1;
    }

    for (int i = 0; i < b->rows; i++)
        for (int j = 0; j < b->cols; j++)
            b->grid[i][j] = (rand() % 100 < 30) ? 1 : 0;
}

void gol_clear(GolBoard *b)
{
    if (!b)
        return;

    for (int i = 0; i < b->rows; i++)
        memset(b->grid[i], 0, (size_t)b->cols * sizeof(int));
}

void gol_step(GolBoard *b)
{
    if (!b || !b->grid)
        return;

    /* doppio buffer */
    int **next = calloc((size_t)b->rows, sizeof(int *));
    if (!next)
        return;

    for (int i = 0; i < b->rows; i++) {
        next[i] = calloc((size_t)b->cols, sizeof(int));
        if (!next[i]) {
            for (int k = 0; k < i; k++)
                free(next[k]);
            free(next);
            return;
        }
    }

    for (int i = 0; i < b->rows; i++) {
        for (int j = 0; j < b->cols; j++) {
            int neighbors = 0;

            for (int di = -1; di <= 1; di++) {
                for (int dj = -1; dj <= 1; dj++) {
                    if (di == 0 && dj == 0)
                        continue;

                    int ni = (i + di + b->rows) % b->rows;
                    int nj = (j + dj + b->cols) % b->cols;

                    neighbors += b->grid[ni][nj];
                }
            }

            if (b->grid[i][j]) {
                /* sopravvivenza: 2 o 3 vicini */
                next[i][j] = (neighbors == 2 || neighbors == 3) ? 1 : 0;
            } else {
                /* nascita: esattamente 3 vicini */
                next[i][j] = (neighbors == 3) ? 1 : 0;
            }
        }
    }

    /* swap buffer */
    for (int i = 0; i < b->rows; i++) {
        free(b->grid[i]);
        b->grid[i] = next[i];
    }
    free(next);
}
