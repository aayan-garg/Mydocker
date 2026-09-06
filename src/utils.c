#include "utils.h"

void bench_timer_start(bench_timer_t *t) {
    clock_gettime(CLOCK_MONOTONIC, &t->start);
}

double bench_timer_elapsed_ms(bench_timer_t *t) {
    clock_gettime(CLOCK_MONOTONIC, &t->end);
    double sec = (double)(t->end.tv_sec - t->start.tv_sec);
    double nsec = (double)(t->end.tv_nsec - t->start.tv_nsec);
    return (sec * 1000.0) + (nsec / 1000000.0);
}
