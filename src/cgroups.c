#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "cgroups.h"
#include "utils.h"

#define CGROUP_BASE_DIR "/sys/fs/cgroup"

static int write_cgroup_file(const char *cgroup_dir, const char *file, const char *value) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", cgroup_dir, file);

    FILE *fp = fopen(path, "w");
    if (!fp) {
        LOG_WARN("Could not open cgroup file '%s' for writing: %s", path, strerror(errno));
        return -1;
    }

    if (fprintf(fp, "%s\n", value) < 0 || fflush(fp) != 0) {
        LOG_WARN("Failed to write value '%s' to '%s': %s", value, path, strerror(errno));
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

static void read_cgroup_file(const char *cgroup_dir, const char *file, char *buf, size_t size) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", cgroup_dir, file);
    FILE *fp = fopen(path, "r");
    if (fp) {
        if (fgets(buf, size, fp)) {
            size_t len = strlen(buf);
            if (len > 0 && buf[len - 1] == '\n') buf[len - 1] = '\0';
        }
        fclose(fp);
    } else {
        snprintf(buf, size, "N/A");
    }
}

int cgroups_setup(pid_t pid, cgroups_config_t *config) {
    if (!config) return 0;

    char cgroup_dir[512];
    snprintf(cgroup_dir, sizeof(cgroup_dir), "%s/mydocker_%d", CGROUP_BASE_DIR, pid);

    LOG_INFO("Initializing cgroups v2 resource limits at '%s'...", cgroup_dir);

    // 1. Create container cgroup directory
    if (mkdir(cgroup_dir, 0755) != 0) {
        perror("mkdir cgroup directory failed");
        return -1;
    }

    // 2. Set Memory Limit if configured (BEFORE attaching PID)
    if (config->mem_max && strlen(config->mem_max) > 0) {
        if (write_cgroup_file(cgroup_dir, "memory.max", config->mem_max) == 0) {
            LOG_INFO("Configured Memory limit (memory.max): %s", config->mem_max);
            write_cgroup_file(cgroup_dir, "memory.swap.max", "0");
        }
    }

    // 3. Set CPU Quota if configured
    if (config->cpu_max && strlen(config->cpu_max) > 0) {
        if (write_cgroup_file(cgroup_dir, "cpu.max", config->cpu_max) == 0) {
            LOG_INFO("Configured CPU limit (cpu.max): %s", config->cpu_max);
        }
    }

    // 4. Set PIDs Limit if configured
    if (config->pids_max > 0) {
        char pids_str[32];
        snprintf(pids_str, sizeof(pids_str), "%d", config->pids_max);
        if (write_cgroup_file(cgroup_dir, "pids.max", pids_str) == 0) {
            LOG_INFO("Configured Process limit (pids.max): %s", pids_str);
        }
    }

    // 5. Attach container child PID to cgroup.procs
    char pid_str[32];
    snprintf(pid_str, sizeof(pid_str), "%d", pid);
    if (write_cgroup_file(cgroup_dir, "cgroup.procs", pid_str) != 0) {
        LOG_ERROR("Failed to attach PID %d to cgroup.procs.", pid);
        return -1;
    }

    return 0;
}

int cgroups_print_stats(pid_t pid) {
    char cgroup_dir[512];
    snprintf(cgroup_dir, sizeof(cgroup_dir), "%s/mydocker_%d", CGROUP_BASE_DIR, pid);

    if (access(cgroup_dir, F_OK) != 0) {
        LOG_ERROR("No active container found with cgroup PID %d at '%s'", pid, cgroup_dir);
        return -1;
    }

    char mem_cur[64], mem_max[64], pids_cur[64], pids_max[64];
    read_cgroup_file(cgroup_dir, "memory.current", mem_cur, sizeof(mem_cur));
    read_cgroup_file(cgroup_dir, "memory.max", mem_max, sizeof(mem_max));
    read_cgroup_file(cgroup_dir, "pids.current", pids_cur, sizeof(pids_cur));
    read_cgroup_file(cgroup_dir, "pids.max", pids_max, sizeof(pids_max));

    unsigned long long bytes = strtoull(mem_cur, NULL, 10);
    double mb = (double)bytes / (1024.0 * 1024.0);

    printf("\n");
    printf("================================================================\n");
    printf("          CONTAINER REAL-TIME METRICS (mydocker stats)          \n");
    printf("================================================================\n");
    printf(" CONTAINER PID : %d\n", pid);
    printf(" MEMORY USAGE  : %.2f MB (%s bytes) / Limit: %s\n", mb, mem_cur, mem_max);
    printf(" ACTIVE TASKS  : %s active processes / Limit: %s\n", pids_cur, pids_max);
    printf("================================================================\n\n");

    return 0;
}

int cgroups_cleanup(pid_t pid) {
    char cgroup_dir[512];
    snprintf(cgroup_dir, sizeof(cgroup_dir), "%s/mydocker_%d", CGROUP_BASE_DIR, pid);

    LOG_INFO("Cleaning up cgroups v2 directory '%s'...", cgroup_dir);
    if (rmdir(cgroup_dir) != 0) {
        return -1;
    }
    return 0;
}
