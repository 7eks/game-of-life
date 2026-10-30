#ifndef RENDER_H
#define RENDER_H

#include <stdint.h>

typedef struct {
    int rows;
    int cols;
} RenderSize;

typedef struct RenderBackend RenderBackend;

struct RenderBackend {
    void   *(*init)(RenderSize size);
    void    (*clear)(void *ctx);
    void    (*render)(void *ctx, int **grid, uint64_t gen);
    void    (*cleanup)(void *ctx);
    int     (*read_key)(void *ctx);
    void    (*set_pause)(void *ctx, int paused);
    int     (*get_pause)(void *ctx);
    void    (*adjust_speed)(void *ctx, int delta);
    int     (*get_speed)(void *ctx);
    void    (*set_blocking)(void *ctx, int blocking);
    void    (*process_events)(void *ctx);
    void    (*get_size)(void *ctx, int *max_rows, int *max_cols);
    void    (*set_size)(void *ctx, RenderSize size);
    void    (*reset_board)(void *ctx);
    void    (*get_board_size)(void *ctx, int *rows, int *cols);
    void    (*clear_grid)(void *ctx);
    int     (*prompt_int)(void *ctx, const char *msg, int min_val, int max_val, int default_val, int *out);
    void    (*clear_prompt)(void *ctx);
};

const RenderBackend *ncurses_backend(void);
const RenderBackend *gtk_backend(void);

#endif /* RENDER_H */
