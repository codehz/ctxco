#pragma once

#include <unistd.h>

typedef struct {
} * ctx_ref_t;

typedef struct _ctx_from_t {
    ctx_ref_t ctx;
    void *priv;
} ctx_from_t;

typedef void (*ctx_func_t)(ctx_from_t from);

ctx_ref_t getctx(char *stackdata, size_t stacksize, ctx_func_t func);

ctx_from_t setctx(ctx_ref_t context, void *priv);