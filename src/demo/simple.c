#include "../ctxco/ctxco.h"

#include <stdio.h>

static unsigned co1_count = 0;
static unsigned co2_count = 0;

void co1(void *priv) {
    co1_count++;
    printf("co1\n");
    ctxco_yield();
    printf("co1 end\n");
    co1_count--;
}
void co2(void *priv) {
    co2_count++;
    printf("co2\n");
    ctxco_yield();
    for (int i = 0; i < 10; i++) {
        printf("co2 spawn co1\n");
        ctxco_start(co1, NULL, 0);
        ctxco_yield();
    }
    printf("co2 end\n");
    co2_count--;
}

int main() {
    printf("main start\n");
    ctxco_init(NULL, NULL);

    ctxco_start(co1, NULL, 0);
    ctxco_start(co2, NULL, 0);

    ctxco_loop();

    ctxco_deinit();
    printf("main end\nco1: %u\nco2: %u\n", co1_count, co2_count);
}