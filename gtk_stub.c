/* Stub per gtk_backend quando GTK non è compilato.
   Fornisce no-op per tutte le callback: main.c fallisce con errore
   chiaro se l'utente passa -g senza GTK disponibile. */
#include <stddef.h>
#include <stdio.h>
#include "render.h"

/* --- No-op per tutte le callback RenderBackend --- */

static void *stub_init(RenderSize size)
{
    (void)size;
    fprintf(stderr, "Errore: backend GTK richiesto ma GTK non è disponibile\n");
    return NULL;
}

static void stub_clear(void *ctx) { (void)ctx; }

static void stub_render(void *ctx, int **grid, uint64_t gen)
{
    (void)ctx;
    (void)grid;
    (void)gen;
}

static void stub_cleanup(void *ctx) { (void)ctx; }

static int stub_read_key(void *ctx)
{
    (void)ctx;
    return -1;
}

static void stub_set_pause(void *ctx, int paused)
{
    (void)ctx;
    (void)paused;
}

static int stub_get_pause(void *ctx)
{
    (void)ctx;
    return 0;
}

static void stub_adjust_speed(void *ctx, int delta)
{
    (void)ctx;
    (void)delta;
}

static int stub_get_speed(void *ctx)
{
    (void)ctx;
    return 250;
}

static void stub_set_blocking(void *ctx, int blocking)
{
    (void)ctx;
    (void)blocking;
}

static void stub_process_events(void *ctx)
{
    (void)ctx;
}

static void stub_get_size(void *ctx, int *max_rows, int *max_cols)
{
    (void)ctx;
    if (max_rows) *max_rows = 60;
    if (max_cols) *max_cols = 80;
}

static void stub_set_size(void *ctx, RenderSize size)
{
    (void)ctx;
    (void)size;
}

static void stub_reset_board(void *ctx)
{
    (void)ctx;
}

static void stub_get_board_size(void *ctx, int *rows, int *cols)
{
    (void)ctx;
    if (rows) *rows = 20;
    if (cols) *cols = 40;
}

static void stub_clear_grid(void *ctx)
{
    (void)ctx;
}

const RenderBackend *gtk_backend(void)
{
    static const RenderBackend backend = {
        .init           = stub_init,
        .clear          = stub_clear,
        .render         = stub_render,
        .cleanup        = stub_cleanup,
        .read_key       = stub_read_key,
        .set_pause      = stub_set_pause,
        .get_pause      = stub_get_pause,
        .adjust_speed   = stub_adjust_speed,
        .get_speed      = stub_get_speed,
        .set_blocking   = stub_set_blocking,
        .process_events = stub_process_events,
        .get_size       = stub_get_size,
        .set_size       = stub_set_size,
        .reset_board    = stub_reset_board,
        .get_board_size = stub_get_board_size,
        .clear_grid     = stub_clear_grid,
    };
    return &backend;
}
