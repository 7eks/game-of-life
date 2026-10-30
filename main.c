#define _POSIX_C_SOURCE 200809L
#include "gol.h"
#include "render.h"
#ifdef USE_NCURSES
#include <ncurses.h>
#endif
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static void print_usage(const char *prog)
{
    fprintf(stderr,
            "Uso: %s [OPTIONS]\n"
            "Options:\n"
            "  -r ROWS   Numero di righe (min: 5, default: 20)\n"
            "  -c COLS   Numero di colonne (min: 5, default: 40)\n"
            "  -g        Usa backend GTK3 (GUI) invece di ncurses\n"
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
    int use_gtk = 0;

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
        case 'g':
            use_gtk = 1;
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

    /* --- Seleziona backend --- */
    const RenderBackend *backend;
    if (use_gtk) {
        backend = gtk_backend();
    } else {
        backend = ncurses_backend();
    }

    if (!backend) {
        fprintf(stderr, "Errore: impossibile inizializzare il backend\n");
        return 1;
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

    /* --- Prompt interattivo se dimensioni non fornite (solo ncurses) --- */
#ifdef USE_NCURSES
    if (!use_gtk && (rows < 0 || cols < 0)) {
        mvprintw(0, 0, "Game of Life - Conway\n");
        if (rows < 0) {
            mvprintw(2, 0, "Inserisci numero di righe (min: 5, max: %d, default: 20): ", max_rows);
            echo();
            curs_set(1);
            rows = 20;
            {
                char buf[64];
                if (wgetnstr(stdscr, buf, sizeof(buf)) == ERR)
                    buf[0] = '\0';
                curs_set(0);
                noecho();
                if (buf[0] != '\0') {
                    errno = 0;
                    long v = strtol(buf, NULL, 10);
                    if (errno != ERANGE && v >= 5 && v <= max_rows)
                        rows = (int)v;
                }
            }
            if (rows < 5)
                rows = 20;
        }
        if (cols < 0) {
            mvprintw(4, 0, "Inserisci numero di colonne (min: 5, max: %d, default: 40): ", max_cols);
            echo();
            curs_set(1);
            cols = 40;
            {
                char buf[64];
                if (wgetnstr(stdscr, buf, sizeof(buf)) == ERR)
                    buf[0] = '\0';
                curs_set(0);
                noecho();
                if (buf[0] != '\0') {
                    errno = 0;
                    long v = strtol(buf, NULL, 10);
                    if (errno != ERANGE && v >= 5 && v <= max_cols)
                        cols = (int)v;
                }
            }
            if (cols < 5)
                cols = 40;
        }
        clear();
    }
#endif

    if (rows < 5)
        rows = 20;
    if (cols < 5)
        cols = 40;

    /* --- Controlla che la griglia entri nel terminale (solo ncurses) --- */
    if (!use_gtk && (rows > max_rows || cols > max_cols)) {
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

        /* Process pending GTK events (no-op for ncurses) */
        if (backend->process_events)
            backend->process_events(ctx);

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
        case 'R':
            /* Resize board (GTK resize) */
            {
                int nrows, ncols;
                backend->get_board_size(ctx, &nrows, &ncols);
                /* Nulla il puntatore al grid: on_draw potrebbe essere in coda */
                backend->clear_grid(ctx);
                gol_free(board);
                board = gol_create(nrows, ncols);
                if (!board) {
                    fprintf(stderr, "Errore: allocazione board fallita\n");
                    running = 0;
                } else {
                    backend->set_size(ctx, (RenderSize){nrows, ncols});
                    gol_randomize(board);
                    gen = 0;
                }
            }
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
            struct timespec ts = {
                .tv_sec  = backend->get_speed(ctx) / 1000,
                .tv_nsec = (backend->get_speed(ctx) % 1000) * 1000000L
            };
            nanosleep(&ts, NULL);
            gol_step(board);
            gen++;
        }
    }

    /* --- Cleanup --- */
    backend->cleanup(ctx);
    gol_free(board);

    return 0;
}
