#ifndef MYDOCKER_CGROUPS_H
#define MYDOCKER_CGROUPS_H

#include <sys/types.h>

typedef struct {
    char *mem_max;  // e.g. "50M" or "52428800"
    char *cpu_max;  // e.g. "20000 100000" (20% CPU)
    int pids_max;   // e.g. 20 (max 20 processes)
} cgroups_config_t;

// Setup cgroups v2 directory and attach child PID
int cgroups_setup(pid_t pid, cgroups_config_t *config);

// Print live container metrics from cgroups v2 files
int cgroups_print_stats(pid_t pid);

// Cleanup cgroups v2 directory on container termination
int cgroups_cleanup(pid_t pid);

#endif // MYDOCKER_CGROUPS_H
