#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    size_t size = 50 * 1024 * 1024; // 50MB
    printf("Allocating and dirtying %zu bytes of memory...\n", size);
    char *ptr = malloc(size);
    if (!ptr) {
        perror("malloc failed");
        return 1;
    }
    // Touch every page to force physical memory allocation
    memset(ptr, 'A', size);
    printf("Successfully allocated 50MB!\n");
    free(ptr);
    return 0;
}
