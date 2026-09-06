#ifndef MYDOCKER_UTILS_H
#define MYDOCKER_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[1;31m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_BLUE    "\033[1;34m"
#define COLOR_MAGENTA "\033[1;35m"
#define COLOR_CYAN    "\033[1;36m"

#define LOG_INFO(fmt, ...) \
    fprintf(stdout, COLOR_GREEN "[INFO]" COLOR_RESET " " fmt "\n", ##__VA_ARGS__)

#define LOG_WARN(fmt, ...) \
    fprintf(stdout, COLOR_YELLOW "[WARN]" COLOR_RESET " " fmt "\n", ##__VA_ARGS__)

#define LOG_ERROR(fmt, ...) \
    fprintf(stderr, COLOR_RED "[ERROR]" COLOR_RESET " " fmt "\n", ##__VA_ARGS__)

#define LOG_CONTAINER(fmt, ...) \
    fprintf(stdout, COLOR_CYAN "[CONTAINER]" COLOR_RESET " " fmt "\n", ##__VA_ARGS__)

// Timing helper
typedef struct {
    struct timespec start;
    struct timespec end;
} bench_timer_t;

void bench_timer_start(bench_timer_t *t);
double bench_timer_elapsed_ms(bench_timer_t *t);

#endif // MYDOCKER_UTILS_H
