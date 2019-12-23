# CTXCO

Minimal coroutine implemented in c.

## API

```c
typedef void (*ctxco_func_t)(void *priv);
typedef void (*ctxco_poller_func_t)(void *priv, ctxco_request_ref_t co);

void ctxco_init(ctxco_poller_func_t poller, void *priv);
void ctxco_deinit();
void ctxco_start(ctxco_func_t func, void *priv, size_t stacksize);
void ctxco_loop();
bool ctxco_yield();
void *ctxco_invoke(void *request);
void ctxco_resume(ctxco_impl_t co, void *ret);
```

## Example

```c
void co1(void *priv) {
    printf("co1\n");
    ctxco_yield();
    printf("co1 end\n");
}
void co2(void *priv) {
    printf("co2\n");
    ctxco_yield();
    for (int i = 0; i < 10; i++) {
        printf("co2 spawn co1\n");
        ctxco_start(co1, NULL, 0);
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