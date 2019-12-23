#pragma once

#include <stdbool.h>
#include <unistd.h>

// macro
#ifdef __GNUC__
#define CTXCO_EXPORT __attribute__((visibility("default")))
#else
#define CTXCO_EXPORT
#endif

#define CTXCO_BLOCK (ctxco_request_ref_t) - 1

// typedef
typedef struct {
} * ctxco_impl_t;

typedef struct {
    ctxco_impl_t ctx;
    void *priv;
} ctxco_request_t, *ctxco_request_ref_t;

typedef void (*ctxco_func_t)(void *priv);
typedef void (*ctxco_poller_func_t)(void *priv, ctxco_request_ref_t co);

// function
CTXCO_EXPORT void ctxco_init(ctxco_poller_func_t poller, void *priv);
CTXCO_EXPORT void ctxco_deinit();
CTXCO_EXPORT void ctxco_start(ctxco_func_t func, void *priv, size_t stacksize);
CTXCO_EXPORT void ctxco_loop();
CTXCO_EXPORT bool ctxco_yield();
CTXCO_EXPORT void *ctxco_invoke(void *request);
CTXCO_EXPORT void ctxco_resume(ctxco_impl_t co, void *ret);