#define _DEFAULT_SOURCE
#include "gol.h"
#include "render.h"
#include <ncurses.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void print_usage(const char *prog)
{
    fprintf(stderr,
            "Uso: %s [OPTIONS]\n"
            "Options:\n"
            "  -r ROWS   Numero di righe (min: 5, default: 20)\n"
            "  -c COLS   Numero di colonne (min: 5, default: 40)\n"
            "  -h        Mostra aiuto\n",
            prog);
}

/* Converte un argomento CLI in int, controllando l'overflow.
   Restituisce 0 in caso di successo, -1 in caso di errore (messaggio gia' stampato). */
static int parse_int_arg(const char *arg, const char *opt, int *out)
{
    errno = 0;
    long v = strtol(arg, NULL, 10);
    if (errno == ERANGE || v < INT_MIN || v > INT_MAX) {
        fprintf(stderr, "Errore: valore fuori range per %s: '%s'\n", opt, arg);
        return -1;
    }
    *out = (int)v;
    return 0;
}

int main(int argc, char *argv[])
{
    const RenderBackend *backend = ncurses_backend();
    if (!backend) {
        fprintf(stderr, "Errore: impossibile inizializzare il backend\n");
        return 1;
    }

    /* --- Parse opzioni CLI --- */
    int rows = -1;
    int cols = -1;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-')
            continue;
        switch (argv[i][1]) {
        case 'r':
            if (i + 1 >= argc) {
                fprintf(stderr, "Errore: -r richiede un argomento\n");
                print_usage(argv[0]);
                return 1;
            }
            if (parse_int_arg(argv[++i], "-r", &rows) != 0) {
                print_usage(argv[0]);
                return 1;
            }
            break;
        case 'c':
            if (i + 1 >= argc) {
                fprintf(stderr, "Errore: -c richiede un argomento\n");
                print_usage(argv[0]);
                return 1;
            }
            if (parse_int_arg(argv[++i], "-c", &cols) != 0) {
                print_usage(argv[0]);
                return 1;
            }
            break;
        case 'h':
            print_usage(argv[0]);
            return 0;
        default:
            fprintf(stderr, "Errore: opzione sconosciuta '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    /* --- Inizializza backend (serve per prompt e get_size) --- */
    void *ctx = backend->init((RenderSize){20, 40});
    if (!ctx) {
        fprintf(stderr, "Errore: impossibile inizializzare il backend\n");
        return 1;
    }

    /* --- Ottieni dimensioni terminali disponibili --- */
    int max_rows = 0, max_cols = 0;
    if (backend->get_size)
        backend->get_size(ctx, &max_rows, &max_cols);

    /* --- Prompt interattivo se dimensioni non fornite --- */
    if (rows < 0 || cols < 0) {
        mvprintw(0, 0, "Game of Life - Conway\n");
        if (rows < 0) {
            mvprintw(2, 0, "Inserisci numero di righe (min: 5, max: %d, default: 20): ", max_rows);
            echo();
            curs_set(1);
            rows = 20;
            scanw("%d", &rows);
            curs_set(0);
            noecho();
            if (rows < 5)
                rows = 20;
        }
        if (cols < 0) {
            mvprintw(4, 0, "Inserisci numero di colonne (min: 5, max: %d, default: 40): ", max_cols);
            echo();
            curs_set(1);
            cols = 40;
            scanw("%d", &cols);
            curs_set(0);
            noecho();
            if (cols < 5)
                cols = 40;
        }
        clear();
    }

    if (rows < 5)
        rows = 20;
    if (cols < 5)
        cols = 40;

    /* --- Controlla che la griglia entri nel terminale --- */
    if (rows > max_rows || cols > max_cols) {
        /* cleanup prima di stampare: altrimenti il testo si mescola ai codici di escape */
        backend->cleanup(ctx);
        fprintf(stderr,
                "Errore: griglia troppo grande (%dx%d) per il terminale (%dx%d)\n",
                rows, cols, max_rows, max_cols);
        return 1;
    }

    /* --- Crea board --- */
    GolBoard *board = gol_create(rows, cols);
    if (!board) {
        backend->cleanup(ctx);
        fprintf(stderr, "Errore: allocazione board fallita\n");
        return 1;
    }

    /* --- Aggiorna dimensione nel contesto render --- */
    backend->set_size(ctx, (RenderSize){rows, cols});

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
