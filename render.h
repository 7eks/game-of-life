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
};

const RenderBackend *ncurses_backend(void);

#endif /* RENDER_H */
