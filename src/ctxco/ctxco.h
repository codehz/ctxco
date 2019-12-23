#pragma once

#include <stdbool.h>
#include <unistd.h>

// macro
#ifndef CTXCO_DEFAULT_STACK_SIZE
#    define CTXCO_DEFAULT_STACK_SIZE 2 * 1024 * 1024
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
void ctxco_init(ctxco_poller_func_t poller, void *priv);
void ctxco_deinit();
void ctxco_start(ctxco_func_t func, void *priv, size_t stacksize);
void ctxco_loop();
bool ctxco_yield();
void *ctxco_invoke(void *request);
void ctxco_resume(ctxco_impl_t co, void *ret);