#ifndef MYDOCKER_CONTAINER_H
#define MYDOCKER_CONTAINER_H

#include <sys/types.h>
#include "cgroups.h"
#include "net.h"

#define CONTAINER_STACK_SIZE (1024 * 1024) // 1MB Stack

typedef struct {
    char *hostname;
    char *rootfs;
    cgroups_config_t cgroups;
    net_config_t net;
    char **argv;
    int argc;
} container_config_t;

int container_run(container_config_t *config);

#endif // MYDOCKER_CONTAINER_H
