#define _DEFAULT_SOURCE
#include "gol.h"
#include "render.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    const RenderBackend *backend = ncurses_backend();
    if (!backend) {
        fprintf(stderr, "Errore: impossibile inizializzare il backend\n");
        return 1;
    }

    /* --- Prompt dimensioni --- */
    mvprintw(0, 0, "Game of Life - Conway\n");
    mvprintw(2, 0, "Inserisci numero di righe (min: 5, default: 20): ");
    echo();
    curs_set(1);
    int rows = 20;
    scanw("%d", &rows);
    curs_set(0);
    noecho();

    if (rows < 5)
        rows = 20;

    mvprintw(4, 0, "Inserisci numero di colonne (min: 5, default: 40): ");
    echo();
    curs_set(1);
    int cols = 40;
    scanw("%d", &cols);
    curs_set(0);
    noecho();

    if (cols < 5)
        cols = 40;

    clear();

    /* --- Crea board --- */
    GolBoard *board = gol_create(rows, cols);
    if (!board) {
        fprintf(stderr, "Errore: allocazione board fallita\n");
        backend->cleanup(backend->init((RenderSize){rows, cols}));
        return 1;
    }

    /* --- Inizializza render --- */
    RenderSize rsize = {rows, cols};
    void *ctx = backend->init(rsize);
    if (!ctx) {
        fprintf(stderr, "Errore: inizializzazione display fallita\n");
        gol_free(board);
        return 1;
    }

    /* --- Randomizza e avvia --- */
    gol_randomize(board);
    uint64_t gen = 0;

    /* --- Ciclo principale --- */
    int running = 1;

    while (running) {
        backend->render(ctx, board->grid, gen);

        backend->set_blocking(ctx, backend->get_pause(ctx));
        int ch = backend->read_key(ctx);

        switch (ch) {
        case 'q':
            running = 0;
            break;
        case ' ':
            backend->set_pause(ctx, !backend->get_pause(ctx));
            break;
        case 'r':
            gol_randomize(board);
            gen = 0;
            break;
        case 'c':
            gol_clear(board);
            gen = 0;
            break;
        case '+':
        case '=':
            backend->adjust_speed(ctx, 1);
            break;
        case '-':
        case '_':
            backend->adjust_speed(ctx, -1);
            break;
        default:
            break;
        }

        if (!backend->get_pause(ctx) && running) {
            usleep(backend->get_speed(ctx) * 1000);
            gol_step(board);
            gen++;
        }
    }

    /* --- Cleanup --- */
    backend->cleanup(ctx);
    gol_free(board);

    return 0;
}
