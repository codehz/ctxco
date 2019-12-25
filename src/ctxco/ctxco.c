#include "ctxco.h"
#include "ctx.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>

#ifndef CTXCO_DEFAULT_STACK_SIZE
#    define CTXCO_DEFAULT_STACK_SIZE 2 * 1024 * 1024
#endif

typedef struct ctxco_t {
    ctx_ref_t ctx;
    ctxco_func_t entry;
    void *priv;
    STAILQ_ENTRY(ctxco_t) next;
} ctxco_t, *ctxco_ref_t;

typedef STAILQ_HEAD(ctxco_list_t, ctxco_t) ctxco_list_t, *ctxco_list_ref_t;

typedef struct ctxco_poller_t {
    ctxco_poller_func_t entry;
    void *priv;
} ctxco_poller_t, *ctxco_poller_ref_t;

typedef struct ctxco_scheduler_t {
    ctxco_t main;
    ctxco_ref_t running;
    ctxco_list_t ready;
    ctxco_list_t dead;
    ctxco_poller_t poller;
} ctxco_scheduler_t, *ctxco_scheduler_ref_t;

static __thread ctxco_scheduler_ref_t global_scheduler = NULL;

static void empty_poller(void *priv, ctxco_request_ref_t co) {
    if (co && co != CTXCO_BLOCK) ctxco_resume(co->ctx, NULL);
}

void ctxco_init(ctxco_poller_func_t poller, void *priv) {
    global_scheduler = calloc(1, sizeof(ctxco_scheduler_t));

    global_scheduler->running = &global_scheduler->main;
    STAILQ_INIT(&global_scheduler->ready);
    STAILQ_INIT(&global_scheduler->dead);
    global_scheduler->poller = (ctxco_poller_t){poller ?: empty_poller, priv};
}

static void free_list(ctxco_list_ref_t list) {
    ctxco_ref_t co_next;
    while ((co_next = STAILQ_FIRST(list))) {
        STAILQ_REMOVE_HEAD(list, next);
        free(co_next);
    }
}

void ctxco_deinit() {
    free_list(&global_scheduler->ready);
    free_list(&global_scheduler->dead);

    free(global_scheduler);
    global_scheduler = NULL;
}

static inline ctxco_ref_t ctxco_next() {
    ctxco_ref_t co_next = STAILQ_NEXT(global_scheduler->running, next);
    return co_next ?: STAILQ_FIRST(&global_scheduler->ready);
}

static void ctxco_switch(ctxco_ref_t target) {
    ctxco_ref_t curco         = global_scheduler->running;
    global_scheduler->running = target;

    ctx_from_t from = setctx(target->ctx, curco);

    ctxco_ref_t co_from = (ctxco_ref_t) from.priv;
    co_from->ctx        = from.ctx;
}

static void ctxco_entry(ctx_from_t from) {
    ctxco_ref_t co_from = (ctxco_ref_t) from.priv;
    co_from->ctx        = from.ctx;

    ctxco_ref_t co = global_scheduler->running;
    co->entry(co->priv);

    STAILQ_REMOVE(&global_scheduler->ready, co, ctxco_t, next);
    STAILQ_INSERT_TAIL(&global_scheduler->dead, co, next);
    ctxco_ref_t co_next = STAILQ_FIRST(&global_scheduler->ready);
    if (co_next) {
        ctxco_switch(co_next);
    } else {
        ctxco_switch(&global_scheduler->main);
    }
}

void ctxco_start(ctxco_func_t func, void *priv, size_t stacksize) {
    if (!stacksize) stacksize = CTXCO_DEFAULT_STACK_SIZE;
    void *buffer    = malloc(stacksize);
    ctxco_ref_t ref = buffer;
    STAILQ_INSERT_TAIL(&global_scheduler->ready, ref, next);
    ref->ctx   = getctx(buffer + sizeof(ctxco_t), stacksize - sizeof(ctxco_t), ctxco_entry);
    ref->entry = func;
    ref->priv  = priv;
}

void ctxco_loop() {
    ctxco_ref_t co_next;
    do {
        while ((co_next = STAILQ_FIRST(&global_scheduler->ready))) {
            ctxco_switch(co_next);
            global_scheduler->poller.entry(global_scheduler->poller.priv, NULL);
            free_list(&global_scheduler->dead);
        }
        global_scheduler->poller.entry(global_scheduler->poller.priv, CTXCO_BLOCK);
        free_list(&global_scheduler->dead);
    } while (STAILQ_FIRST(&global_scheduler->ready));
}

bool ctxco_yield() {
    ctxco_ref_t co_next = ctxco_next();
    ctxco_ref_t co      = global_scheduler->running;
    if (co_next && co_next != co) {
        ctxco_switch(co_next);
        free_list(&global_scheduler->dead);
        return true;
    }
    return false;
}

void *ctxco_invoke(int op, ...) {
    ctxco_ref_t co = global_scheduler->running;

    ctxco_request_t req = {(ctxco_impl_t) co, op};
    STAILQ_REMOVE(&global_scheduler->ready, co, ctxco_t, next);
    ctxco_ref_t co_next = STAILQ_FIRST(&global_scheduler->ready);

    ctxco_poller_ref_t poller = &global_scheduler->poller;
    va_start(req.arg, op);
    poller->entry(poller->priv, &req);
    va_end(req.arg);
    if (co_next) {
        ctxco_switch(co_next);
    } else {
        ctxco_switch(&global_scheduler->main);
    }
    return co->priv;
}

void ctxco_resume(ctxco_impl_t co_, void *ret) {
    ctxco_ref_t co = (ctxco_ref_t) co_;
    co->priv       = ret;
    STAILQ_INSERT_HEAD(&global_scheduler->ready, co, next);
}