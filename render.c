#include "render.h"
#include <ncurses.h>
#include <stdlib.h>

typedef struct {
    RenderSize size;
    WINDOW    *win;
    int       paused;
    int       speed;
    uint64_t  gen;
} RenderCtx;

static void *ncurses_init(RenderSize size)
{
    /* initscr() fallisce se TERM e' assente/invalido: stdscr sarebbe NULL */
    if (initscr() == NULL)
        return NULL;

    curs_set(0);
    noecho();
    nodelay(stdscr, FALSE);
    keypad(stdscr, TRUE);
    cbreak();

    RenderCtx *ctx = calloc(1, sizeof(RenderCtx));
    if (!ctx) {
        endwin();
        return NULL;
    }

    ctx->size = size;
    ctx->win  = stdscr;
    ctx->paused = 0;
    ctx->speed  = 250;  /* 250 ms default */
    ctx->gen    = 0;

    return ctx;
}

static void ncurses_clear(void *vctx)
{
    (void)vctx;
    erase();
}

static void ncurses_render(void *vctx, int **grid, uint64_t gen)
{
    RenderCtx *ctx = (RenderCtx *)vctx;
    if (!ctx || !grid)
        return;

    int maxrow, maxcol;
    getmaxyx(stdscr, maxrow, maxcol);

    /* area giocabile: lascia 2 righe in basso per info */
    int avail_rows = maxrow - 2;
    int avail_cols = maxcol - 1;

    if (avail_rows <= 0 || avail_cols <= 0)
        return;

    /* calcola offset per centrare la griglia */
    int off_r = (avail_rows > ctx->size.rows)
        ? (avail_rows - ctx->size.rows) / 2
        : 0;
    int off_c = (avail_cols > ctx->size.cols)
        ? (avail_cols - ctx->size.cols) / 2
        : 0;

    /* disegna griglia */
    for (int i = 0; i < ctx->size.rows && i + off_r < maxrow; i++) {
        for (int j = 0; j < ctx->size.cols && j + off_c < maxcol; j++) {
            if (move(i + off_r, j + off_c) == OK) {
                chtype ch = grid[i][j] ? '#' : ' ';
                addch(ch);
            }
        }
    }

    /* riga info in basso */
    char buf[128];
    int info_row = maxrow - 2;

    if (info_row >= 0 && move(info_row, 0) == OK) {
        if (ctx->paused)
            snprintf(buf, sizeof(buf), "PAUSA  |  Gen: %llu",
                     (unsigned long long)gen);
        else
            snprintf(buf, sizeof(buf), "Gen: %llu",
                     (unsigned long long)gen);
        addstr(buf);
        clrtoeol();
    }

    if (info_row + 1 >= 0 && move(info_row + 1, 0) == OK) {
        snprintf(buf, sizeof(buf),
                 "Spazio:pausa  r:random  c:clear  +/-:velocita  ");
        addstr(buf);

        /* barra velocita: # = veloce, - = lento, 5 livelli */
        int bars = (500 - ctx->speed + 50) / 100;
        if (bars < 0)
            bars = 0;
        if (bars > 5)
            bars = 5;

        for (int i = 0; i < 5; i++)
            addch(i < bars ? '#' : '-');

        addstr("  q:esci");
        clrtoeol();
    }

    refresh();
}

static int ncurses_read_key(void *vctx)
{
    (void)vctx;
    return getch();
}

static void ncurses_set_pause(void *vctx, int paused)
{
    RenderCtx *ctx = (RenderCtx *)vctx;
    if (ctx)
        ctx->paused = paused;
}

static int ncurses_get_pause(void *vctx)
{
    RenderCtx *ctx = (RenderCtx *)vctx;
    return ctx ? ctx->paused : 0;
}

static void ncurses_adjust_speed(void *vctx, int delta)
{
    RenderCtx *ctx = (RenderCtx *)vctx;
    if (!ctx)
        return;
    if (delta > 0 && ctx->speed > 50)
        ctx->speed -= 100;
    else if (delta < 0 && ctx->speed < 500)
        ctx->speed += 100;
}

static int ncurses_get_speed(void *vctx)
{
    RenderCtx *ctx = (RenderCtx *)vctx;
    return ctx ? ctx->speed : 250;
}

static void ncurses_set_blocking(void *vctx, int blocking)
{
    RenderCtx *ctx = (RenderCtx *)vctx;
    if (!ctx || !ctx->win)
        return;
    nodelay(ctx->win, !blocking);
}

static void ncurses_get_size(void *vctx, int *max_rows, int *max_cols)
{
    (void)vctx;
    int maxrow, maxcol;
    getmaxyx(stdscr, maxrow, maxcol);

    /* riserva 2 righe in basso per info, 1 colonna di margine */
    if (max_rows)
        *max_rows = (maxrow > 2) ? maxrow - 2 : 0;
    if (max_cols)
        *max_cols = (maxcol > 1) ? maxcol - 1 : 0;
}

static void ncurses_set_size(void *vctx, RenderSize size)
{
    RenderCtx *ctx = (RenderCtx *)vctx;
    if (ctx)
        ctx->size = size;
}

static void ncurses_cleanup(void *vctx)
{
    RenderCtx *ctx = (RenderCtx *)vctx;
    endwin();
    free(ctx);
}

static const RenderBackend backend = {
    .init         = ncurses_init,
    .clear        = ncurses_clear,
    .render       = ncurses_render,
    .cleanup      = ncurses_cleanup,
    .read_key     = ncurses_read_key,
    .set_pause    = ncurses_set_pause,
    .get_pause    = ncurses_get_pause,
    .adjust_speed = ncurses_adjust_speed,
    .get_speed    = ncurses_get_speed,
    .set_blocking = ncurses_set_blocking,
    .get_size     = ncurses_get_size,
    .set_size     = ncurses_set_size,
};

const RenderBackend *ncurses_backend(void)
{
    return &backend;
}
