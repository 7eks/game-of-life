#include "render.h"
#include "gol.h"
#include <ncurses.h>
#include <errno.h>
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
    ctx->speed  = SPEED_DEFAULT;
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
        int bars = (SPEED_MAX - ctx->speed + SPEED_STEP / 2) / SPEED_STEP;
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
    if (delta > 0 && ctx->speed > SPEED_MIN)
        ctx->speed -= SPEED_STEP;
    else if (delta < 0 && ctx->speed < SPEED_MAX)
        ctx->speed += SPEED_STEP;
}

static int ncurses_get_speed(void *vctx)
{
    RenderCtx *ctx = (RenderCtx *)vctx;
    return ctx ? ctx->speed : SPEED_DEFAULT;
}

static void ncurses_set_blocking(void *vctx, int blocking)
{
    RenderCtx *ctx = (RenderCtx *)vctx;
    if (!ctx || !ctx->win)
        return;
    nodelay(ctx->win, !blocking);
}

static void ncurses_process_events(void *vctx)
{
    (void)vctx;
    /* ncurses gestisce l'input in modo sincrono, nessun evento da processare */
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

static void ncurses_reset_board(void *vctx)
{
    /* ncurses: resize non supportato, no-op */
    (void)vctx;
}

static void ncurses_get_board_size(void *vctx, int *rows, int *cols)
{
    RenderCtx *ctx = (RenderCtx *)vctx;
    if (!ctx) {
        if (rows) *rows = 20;
        if (cols) *cols = 40;
        return;
    }
    if (rows) *rows = ctx->size.rows;
    if (cols) *cols = ctx->size.cols;
}

static void ncurses_clear_grid(void *vctx)
{
    /* ncurses: nessun puntatore grid separato, no-op */
    (void)vctx;
}

static int ncurses_prompt_int(void *vctx, const char *msg, int min_val, int max_val, int default_val, int *out)
{
    RenderCtx *ctx = (RenderCtx *)vctx;
    if (!ctx || !msg || !out)
        return -1;

    int maxrow, maxcol;
    getmaxyx(stdscr, maxrow, maxcol);
    (void)maxcol;

    int prompt_row = (ctx->size.rows < maxrow - 2) ? ctx->size.rows : maxrow - 3;
    if (prompt_row < 0)
        prompt_row = maxrow - 3;

    char buf[64];
    snprintf(buf, sizeof(buf), "%s (min: %d, max: %d, default: %d): ", msg, min_val, max_val, default_val);

    if (prompt_row >= 0 && move(prompt_row, 0) == OK) {
        addstr(buf);
        clrtoeol();
    }

    echo();
    curs_set(1);
    wgetnstr(stdscr, buf, sizeof(buf));
    curs_set(0);
    noecho();

    if (buf[0] == '\0') {
        *out = default_val;
        return 0;
    }

    errno = 0;
    char *endptr = NULL;
    long v = strtol(buf, &endptr, 10);
    if (errno != ERANGE && *endptr == '\0' && v >= min_val && v <= max_val) {
        *out = (int)v;
        return 0;
    }

    *out = default_val;
    return 0;
}

static void ncurses_clear_prompt(void *vctx)
{
    RenderCtx *ctx = (RenderCtx *)vctx;
    if (!ctx)
        return;

    int maxrow, maxcol;
    getmaxyx(stdscr, maxrow, maxcol);
    (void)maxcol;

    int prompt_row = (ctx->size.rows < maxrow - 2) ? ctx->size.rows : maxrow - 3;
    if (prompt_row < 0)
        prompt_row = maxrow - 3;

    if (prompt_row >= 0 && move(prompt_row, 0) == OK)
        clrtoeol();
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
    .process_events = ncurses_process_events,
    .get_size     = ncurses_get_size,
    .set_size     = ncurses_set_size,
    .reset_board  = ncurses_reset_board,
    .get_board_size = ncurses_get_board_size,
    .clear_grid     = ncurses_clear_grid,
    .prompt_int     = ncurses_prompt_int,
    .clear_prompt   = ncurses_clear_prompt,
};

const RenderBackend *ncurses_backend(void)
{
    return &backend;
}
