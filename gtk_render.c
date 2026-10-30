#include "render.h"
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

/* --- Costanti --- */
#define CELL_SIZE       12
#define CELL_PADDING    1
#define SPEED_MIN       50
#define SPEED_MAX       500
#define SPEED_STEP      100
#define SPEED_DEFAULT   250

/* --- Contesto privato GTK --- */

/* Forward declaration */
static void gtk_backend_reset_board(void *ctx);

typedef struct {
    RenderSize  size;
    int         paused;
    int         speed;
    int         last_key;
    uint64_t    gen;

    GtkWidget  *window;
    GtkWidget  *menubar;
    GtkWidget  *drawing_area;
    GtkWidget  *status_bar;
    GtkWidget  *status_label;

    /* Dimensione board corrente (può cambiare al resize) */
    int         cur_rows;
    int         cur_cols;

    /* Puntatore al grid corrente per il rendering */
    int        **grid;
} GtkRenderCtx;

/* --- Funzioni di supporto --- */

/* Aggiorna la status bar */
static void update_status(GtkRenderCtx *ctx)
{
    if (!ctx->status_label)
        return;

    char buf[256];
    int bars = (SPEED_MAX - ctx->speed + SPEED_STEP / 2) / SPEED_STEP;
    if (bars < 0) bars = 0;
    if (bars > 5) bars = 5;

    char speed_bar[8] = "-----";
    for (int i = 0; i < bars; i++)
        speed_bar[i] = '#';

    snprintf(buf, sizeof(buf),
             "Gen: %llu  |  %s  |  %s  |  q:esci",
             (unsigned long long)ctx->gen,
             ctx->paused ? "PAUSA" : "IN CORSA",
             speed_bar);

    gtk_label_set_text(GTK_LABEL(ctx->status_label), buf);
}

/* Callback drawing area: renderizza la griglia */
static gboolean on_draw(GtkWidget *widget, cairo_t *cr, void *vctx)
{
    GtkRenderCtx *ctx = (GtkRenderCtx *)vctx;
    if (!ctx || !ctx->grid)
        return FALSE;

    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);

    /* Sfondo finestra */
    cairo_set_source_rgba(cr, 0.102, 0.102, 0.180, 1.0);
    cairo_paint(cr);

    int total_cell = CELL_SIZE + CELL_PADDING;
    int grid_w = ctx->cur_cols * total_cell;
    int grid_h = ctx->cur_rows * total_cell;

    /* Offset per centrare la griglia */
    int off_x = (width - grid_w) / 2;
    int off_y = (height - grid_h) / 2;
    if (off_x < 0) off_x = 0;
    if (off_y < 0) off_y = 0;

    cairo_save(cr);
    cairo_translate(cr, off_x, off_y);

    /* Disegna tutte le celle */
    for (int i = 0; i < ctx->size.rows; i++) {
        for (int j = 0; j < ctx->size.cols; j++) {
            int x = j * total_cell;
            int y = i * total_cell;

            /* Sfondo cella morta */
            cairo_set_source_rgba(cr, 0.06, 0.06, 0.14, 1.0);
            cairo_rectangle(cr, x, y, CELL_SIZE, CELL_SIZE);
            cairo_fill(cr);

            /* Bordo */
            cairo_set_source_rgba(cr, 0.09, 0.13, 0.24, 1.0);
            cairo_set_line_width(cr, 0.5);
            cairo_rectangle(cr, x, y, CELL_SIZE, CELL_SIZE);
            cairo_stroke(cr);

            /* Cella viva */
            if (ctx->grid && ctx->grid[i] && ctx->grid[i][j]) {
                cairo_set_source_rgba(cr, 0.298, 0.678, 0.314, 1.0);
                cairo_rectangle(cr, x, y, CELL_SIZE, CELL_SIZE);
                cairo_fill(cr);
            }
        }
    }

    cairo_restore(cr);
    return FALSE;
}

/* Callback resize finestra */
static gboolean on_configure(GtkWidget *widget, GdkEventConfigure *event, void *vctx)
{
    GtkRenderCtx *ctx = (GtkRenderCtx *)vctx;
    if (!ctx)
        return FALSE;

    (void)event;

    /* Ricalcola rows/cols in base alla dimensione della drawing area */
    int avail_w = gtk_widget_get_allocated_width(widget);
    int avail_h = gtk_widget_get_allocated_height(widget);

    int total_cell = CELL_SIZE + CELL_PADDING;
    int new_cols = avail_w / total_cell;
    int new_rows = avail_h / total_cell;

    if (new_cols < 5) new_cols = 5;
    if (new_rows < 5) new_rows = 5;

    if (new_cols != ctx->cur_cols || new_rows != ctx->cur_rows) {
        ctx->cur_cols = new_cols;
        ctx->cur_rows = new_rows;

        /* on_configure firea anche alla creazione finestra (ctx->grid == NULL).
           Ignorare: la board non esiste ancora, verrà creata dal main loop. */
        if (!ctx->grid)
            return FALSE;

        /* Richiede al main loop di ricreare la board con nuove dimensioni */
        if (ctx->size.rows != new_rows || ctx->size.cols != new_cols)
            gtk_backend_reset_board(ctx);
        else
            gtk_widget_queue_draw(widget);
    }

    return FALSE;
}

/* Callback click mouse: toggle cella (solo in pausa) */
static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, void *vctx)
{
    GtkRenderCtx *ctx = (GtkRenderCtx *)vctx;
    if (!ctx || !ctx->paused)
        return FALSE;

    if (event->button != 1) /* Solo click sinistro */
        return FALSE;

    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);

    int total_cell = CELL_SIZE + CELL_PADDING;
    int grid_w = ctx->cur_cols * total_cell;
    int grid_h = ctx->cur_rows * total_cell;

    int off_x = (width - grid_w) / 2;
    int off_y = (height - grid_h) / 2;
    if (off_x < 0) off_x = 0;
    if (off_y < 0) off_y = 0;

    int mx = (int)event->x - off_x;
    int my = (int)event->y - off_y;

    int col = mx / total_cell;
    int row = my / total_cell;

    /* Controlla che il click sia dentro la griglia di visualizzazione */
    if (row < 0 || row >= ctx->cur_rows || col < 0 || col >= ctx->cur_cols)
        return FALSE;

    /* Toggle cella: verifica anche che rientri nelle dimensioni reali della board */
    if (ctx->grid && row < ctx->size.rows && col < ctx->size.cols && ctx->grid[row])
        ctx->grid[row][col] = !ctx->grid[row][col];

    gtk_widget_queue_draw(widget);
    return TRUE;
}

/* Callback tastiera */
static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, void *vctx)
{
    GtkRenderCtx *ctx = (GtkRenderCtx *)vctx;
    if (!ctx)
        return FALSE;

    (void)widget;

    guint key = event->keyval;

    /* F11: fullscreen toggle */
    if (key == GDK_KEY_F11) {
        gtk_window_fullscreen(GTK_WINDOW(ctx->window));
        return TRUE;
    }

    /* Imposta last_key per il main loop */
    switch (key) {
    case GDK_KEY_q:
        ctx->last_key = 'q';
        break;
    case GDK_KEY_space:
        ctx->last_key = ' ';
        break;
    case GDK_KEY_r:
        ctx->last_key = 'r';
        break;
    case GDK_KEY_c:
        ctx->last_key = 'c';
        break;
    case GDK_KEY_plus:
    case GDK_KEY_equal:
        ctx->last_key = '+';
        break;
    case GDK_KEY_minus:
    case GDK_KEY_underscore:
        ctx->last_key = '-';
        break;
    default:
        return FALSE;
    }

    return TRUE;
}

/* Callback menu: Random */
static void on_menu_random(GtkMenuItem *menu, void *vctx)
{
    (void)menu;
    GtkRenderCtx *ctx = (GtkRenderCtx *)vctx;
    if (ctx)
        ctx->last_key = 'r';
}

/* Callback menu: Quit */
static void on_menu_quit(GtkMenuItem *menu, void *vctx)
{
    (void)menu;
    GtkRenderCtx *ctx = (GtkRenderCtx *)vctx;
    if (ctx)
        ctx->last_key = 'q';
}

/* Callback menu: Clear */
static void on_menu_clear(GtkMenuItem *menu, void *vctx)
{
    (void)menu;
    GtkRenderCtx *ctx = (GtkRenderCtx *)vctx;
    if (ctx)
        ctx->last_key = 'c';
}

/* Callback menu: Pause/Play */
static void on_menu_pause(GtkMenuItem *menu, void *vctx)
{
    (void)menu;
    GtkRenderCtx *ctx = (GtkRenderCtx *)vctx;
    if (ctx)
        ctx->last_key = ' ';
}

/* Callback menu: Speed Up */
static void on_menu_speed_up(GtkMenuItem *menu, void *vctx)
{
    (void)menu;
    GtkRenderCtx *ctx = (GtkRenderCtx *)vctx;
    if (ctx)
        ctx->last_key = '+';
}

/* Callback menu: Speed Down */
static void on_menu_speed_down(GtkMenuItem *menu, void *vctx)
{
    (void)menu;
    GtkRenderCtx *ctx = (GtkRenderCtx *)vctx;
    if (ctx)
        ctx->last_key = '-';
}

/* Callback menu: Shortcuts */
static void on_menu_shortcuts(GtkMenuItem *menu, void *vctx)
{
    (void)menu;
    GtkRenderCtx *ctx = (GtkRenderCtx *)vctx;
    if (!ctx)
        return;

    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(ctx->window),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_OK,
        "Scorciatoie tastiera:\n\n"
        "  F11        Toggle fullscreen\n"
        "  Spazio     Pausa / Ripresa\n"
        "  r          Randomizza\n"
        "  c          Pulisci\n"
        "  + / =      Aumenta velocità\n"
        "  - / _      Diminuisci velocità\n"
        "  q          Esci\n"
        "  Click      Toggle cella (solo in pausa)");

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

/* Crea la barra dei menu */
static GtkWidget *create_menu_bar(GtkRenderCtx *ctx)
{
    /* --- File --- */
    /* --- Popup menu --- */
    GtkWidget *file_popup = gtk_menu_new();

    GtkWidget *quit_item = gtk_menu_item_new_with_label("Esci");
    g_signal_connect(G_OBJECT(quit_item), "activate",
                     G_CALLBACK(on_menu_quit), ctx);
    gtk_menu_shell_append(GTK_MENU_SHELL(file_popup), quit_item);

    GtkWidget *game_popup = gtk_menu_new();

    GtkWidget *random_item = gtk_menu_item_new_with_label("Randomizza");
    g_signal_connect(G_OBJECT(random_item), "activate",
                     G_CALLBACK(on_menu_random), ctx);
    gtk_menu_shell_append(GTK_MENU_SHELL(game_popup), random_item);

    GtkWidget *clear_item = gtk_menu_item_new_with_label("Pulisci");
    g_signal_connect(G_OBJECT(clear_item), "activate",
                     G_CALLBACK(on_menu_clear), ctx);
    gtk_menu_shell_append(GTK_MENU_SHELL(game_popup), clear_item);

    GtkWidget *pause_item = gtk_menu_item_new_with_label("Pausa/Ripresa");
    g_signal_connect(G_OBJECT(pause_item), "activate",
                     G_CALLBACK(on_menu_pause), ctx);
    gtk_menu_shell_append(GTK_MENU_SHELL(game_popup), pause_item);

    GtkWidget *view_popup = gtk_menu_new();

    GtkWidget *speed_up_item = gtk_menu_item_new_with_label("Velocità +");
    g_signal_connect(G_OBJECT(speed_up_item), "activate",
                     G_CALLBACK(on_menu_speed_up), ctx);
    gtk_menu_shell_append(GTK_MENU_SHELL(view_popup), speed_up_item);

    GtkWidget *speed_down_item = gtk_menu_item_new_with_label("Velocità -");
    g_signal_connect(G_OBJECT(speed_down_item), "activate",
                     G_CALLBACK(on_menu_speed_down), ctx);
    gtk_menu_shell_append(GTK_MENU_SHELL(view_popup), speed_down_item);

    GtkWidget *help_popup = gtk_menu_new();

    GtkWidget *shortcuts_item = gtk_menu_item_new_with_label("Scorciatoie");
    g_signal_connect(G_OBJECT(shortcuts_item), "activate",
                     G_CALLBACK(on_menu_shortcuts), ctx);
    gtk_menu_shell_append(GTK_MENU_SHELL(help_popup), shortcuts_item);

    /* --- Menu bar --- */
    GtkWidget *menubar = gtk_menu_bar_new();

    GtkWidget *file_item = gtk_menu_item_new_with_label("File");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_item), file_popup);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), file_item);

    GtkWidget *game_item = gtk_menu_item_new_with_label("Game");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(game_item), game_popup);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), game_item);

    GtkWidget *view_item = gtk_menu_item_new_with_label("View");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(view_item), view_popup);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), view_item);

    GtkWidget *help_item = gtk_menu_item_new_with_label("Help");
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(help_item), help_popup);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), help_item);

    return menubar;
}

/* --- Implementazione RenderBackend --- */

static void *gtk_backend_init(RenderSize size)
{
    gtk_init(NULL, NULL);

    GtkRenderCtx *ctx = calloc(1, sizeof(GtkRenderCtx));
    if (!ctx)
        return NULL;

    ctx->size = size;
    ctx->paused = 0;
    ctx->speed = SPEED_DEFAULT;
    ctx->last_key = -1;
    ctx->gen = 0;
    ctx->cur_rows = size.rows;
    ctx->cur_cols = size.cols;
    ctx->grid = NULL;

    /* Crea finestra */
    ctx->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(ctx->window), "Game of Life — Conway");
    gtk_window_set_default_size(GTK_WINDOW(ctx->window),
                                size.cols * (CELL_SIZE + CELL_PADDING) + 40,
                                size.rows * (CELL_SIZE + CELL_PADDING) + 80);
    gtk_window_set_resizable(GTK_WINDOW(ctx->window), TRUE);

    /* Layout verticale: menu | drawing | status */
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(ctx->window), vbox);

    /* Menu */
    ctx->menubar = create_menu_bar(ctx);
    gtk_box_pack_start(GTK_BOX(vbox), ctx->menubar, FALSE, FALSE, 0);

    /* Drawing area */
    ctx->drawing_area = gtk_drawing_area_new();
    /* Abilita eventi mouse e tastiera sulla drawing area */
    gtk_widget_add_events(ctx->drawing_area, GDK_BUTTON_PRESS_MASK | GDK_KEY_PRESS_MASK);
    gtk_box_pack_start(GTK_BOX(vbox), ctx->drawing_area, TRUE, TRUE, 0);

    /* Status bar */
    ctx->status_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    ctx->status_label = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(ctx->status_bar), ctx->status_label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), ctx->status_bar, FALSE, FALSE, 0);

    /* Connessione signals */
    g_signal_connect(G_OBJECT(ctx->drawing_area), "draw",
                     G_CALLBACK(on_draw), ctx);
    g_signal_connect(G_OBJECT(ctx->drawing_area), "configure-event",
                     G_CALLBACK(on_configure), ctx);
    g_signal_connect(G_OBJECT(ctx->drawing_area), "button-press-event",
                     G_CALLBACK(on_button_press), ctx);
    g_signal_connect(G_OBJECT(ctx->window), "key-press-event",
                     G_CALLBACK(on_key_press), ctx);

    gtk_widget_show_all(ctx->window);

    return ctx;
}

static void gtk_backend_clear(void *vctx)
{
    (void)vctx;
    /* In GTK, clear è gestito dal main loop con gol_clear() */
}

static void gtk_backend_render(void *vctx, int **grid, uint64_t gen)
{
    GtkRenderCtx *ctx = (GtkRenderCtx *)vctx;
    if (!ctx || !ctx->drawing_area)
        return;

    /* Aggiorna riferimento al grid e generazione */
    ctx->grid = grid;
    ctx->gen = gen;

    /* Aggiorna status bar */
    update_status(ctx);

    /* Refresh drawing area */
    gtk_widget_queue_draw(ctx->drawing_area);
}

static void gtk_backend_cleanup(void *vctx)
{
    GtkRenderCtx *ctx = (GtkRenderCtx *)vctx;
    if (!ctx)
        return;

    if (ctx->window) {
        gtk_widget_destroy(ctx->window);
        /* Processa eventi residui dopo la distruzione per evitare use-after-free */
        while (gtk_events_pending())
            gtk_main_iteration_do(FALSE);
    }
    free(ctx);
}

static int gtk_backend_read_key(void *vctx)
{
    GtkRenderCtx *ctx = (GtkRenderCtx *)vctx;
    if (!ctx)
        return -1;

    int key = ctx->last_key;
    ctx->last_key = -1;
    return key;
}

static void gtk_backend_set_pause(void *vctx, int paused)
{
    GtkRenderCtx *ctx = (GtkRenderCtx *)vctx;
    if (ctx)
        ctx->paused = paused;
}

static int gtk_backend_get_pause(void *vctx)
{
    GtkRenderCtx *ctx = (GtkRenderCtx *)vctx;
    return ctx ? ctx->paused : 0;
}

static void gtk_backend_adjust_speed(void *vctx, int delta)
{
    GtkRenderCtx *ctx = (GtkRenderCtx *)vctx;
    if (!ctx)
        return;
    if (delta > 0 && ctx->speed > SPEED_MIN)
        ctx->speed -= SPEED_STEP;
    else if (delta < 0 && ctx->speed < SPEED_MAX)
        ctx->speed += SPEED_STEP;
}

static int gtk_backend_get_speed(void *vctx)
{
    GtkRenderCtx *ctx = (GtkRenderCtx *)vctx;
    return ctx ? ctx->speed : SPEED_DEFAULT;
}

static void gtk_backend_set_blocking(void *vctx, int blocking)
{
    /* GTK è event-driven, non usa blocking/non-blocking */
    (void)vctx;
    (void)blocking;
}

static void gtk_backend_process_events(void *vctx)
{
    (void)vctx;
    /* Process pending GTK events without blocking */
    while (gtk_events_pending())
        gtk_main_iteration_do(FALSE);
}

static void gtk_backend_get_size(void *vctx, int *max_rows, int *max_cols)
{
    (void)vctx;
    /* In GTK, le dimensioni sono dynamiche. */
    if (max_rows) *max_rows = 60;
    if (max_cols) *max_cols = 80;
}

static void gtk_backend_set_size(void *vctx, RenderSize size)
{
    GtkRenderCtx *ctx = (GtkRenderCtx *)vctx;
    if (!ctx)
        return;
    ctx->size = size;
    /* cur_rows/cols restano le dimensioni di display (aggiornate da on_configure) */
}

static void gtk_backend_reset_board(void *vctx)
{
    GtkRenderCtx *ctx = (GtkRenderCtx *)vctx;
    if (!ctx)
        return;
    /* Segnala al main loop di ricreare la board.
       Non aggiornare ctx->size: resta pari alle dimensioni reali della board. */
    ctx->last_key = 'R';
}

static void gtk_backend_get_board_size(void *vctx, int *rows, int *cols)
{
    GtkRenderCtx *ctx = (GtkRenderCtx *)vctx;
    if (!ctx) {
        if (rows) *rows = 20;
        if (cols) *cols = 40;
        return;
    }
    if (rows) *rows = ctx->cur_rows;
    if (cols) *cols = ctx->cur_cols;
}

static void gtk_backend_clear_grid(void *vctx)
{
    GtkRenderCtx *ctx = (GtkRenderCtx *)vctx;
    if (ctx)
        ctx->grid = NULL;
}

/* Static backend struct */
static const RenderBackend backend = {
    .init         = gtk_backend_init,
    .clear        = gtk_backend_clear,
    .render       = gtk_backend_render,
    .cleanup      = gtk_backend_cleanup,
    .read_key     = gtk_backend_read_key,
    .set_pause    = gtk_backend_set_pause,
    .get_pause    = gtk_backend_get_pause,
    .adjust_speed = gtk_backend_adjust_speed,
    .get_speed    = gtk_backend_get_speed,
    .set_blocking = gtk_backend_set_blocking,
    .process_events = gtk_backend_process_events,
    .get_size     = gtk_backend_get_size,
    .set_size     = gtk_backend_set_size,
    .reset_board  = gtk_backend_reset_board,
    .get_board_size = gtk_backend_get_board_size,
    .clear_grid   = gtk_backend_clear_grid,
};

const RenderBackend *gtk_backend(void)
{
    return &backend;
}
