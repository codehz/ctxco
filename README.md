# CTXCO

Minimal coroutine implemented in c.

Only support linux for now!

Experiment project, **DO NOT** use it in production.

## API

```c
typedef void (*ctxco_func_t)(void *priv);
typedef void (*ctxco_poller_func_t)(void *priv, ctxco_request_ref_t co);

void ctxco_init(ctxco_poller_func_t poller, void *priv);
void ctxco_deinit();
void ctxco_start(ctxco_func_t func, void *priv, size_t stacksize);
void ctxco_loop();
bool ctxco_yield();
void *ctxco_invoke(int op, ...);
void ctxco_resume(ctxco_impl_t co, void *ret);
```

* ctxco_init: initialize the library (per thread)

you can pass an ad-hoc poller function for it, see src/demo/epoll.c (*PS: Not an efficient implementation*)

* ctxco_deinit: clean up
* ctxco_start: start coroutine
* ctxco_loop: start main loop (will return when all coroutine exit)
* ctxco_yield: switch to another coroutine
* ctxco_invoke: sending custom poll request to your poller function
* ctxco_resume: send the coroutine to ready queue

## Build

This project use [xmake](https://xmake.io) as build system. You can just install xmake and run `xmake build -a` to build.

## Example

```c
// coroutine entry 01
void co1(void *priv) {
    printf("co1\n");
    // switch to other coroutine
    ctxco_yield();
    printf("co1 end\n");
}
// coroutine entry 02
void co2(void *priv) {
    printf("co2\n");
    // switch to other coroutine
    ctxco_yield();
    for (int i = 0; i < 10; i++) {
        printf("co2 spawn co1\n");
        // spawn a new coroutine in co2 coroutine
        ctxco_start(co1, NULL, 0);
        // switch to other coroutine
        ctxco_yield();
    }
    printf("co2 end\n");
}

int main() {
    printf("main start\n");
    ctxco_init(NULL, NULL);

    ctxco_start(co1, NULL, 0);
    ctxco_start(co2, NULL, 0);

    ctxco_loop();

    ctxco_deinit();
    printf("main end\n");
}
```

## License

MIT License
Copyright (c) 2019 Code Hz

## Thanks

* Boost.context
* tbox