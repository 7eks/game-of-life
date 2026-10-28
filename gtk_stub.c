/* Stub per gtk_backend quando GTK non è compilato.
   Restituisce NULL, quindi main.c fallisce con errore chiaro. */
#include <stddef.h>
#include "render.h"

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
        .reset_board  = stub_reset_board,
        .get_board_size = stub_get_board_size,
        .clear_grid   = stub_clear_grid,
    };
    return &backend;
}
