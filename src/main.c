#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cgroups.h"
#include "container.h"
#include "net.h"
#include "utils.h"

static void print_usage(const char *prog_name) {
    printf("Minimal Linux Container Runtime (mydocker) - v0.6.0 (Stage 6: Security & Stats)\n");
    printf("Usage:\n");
    printf("  sudo %s run [OPTIONS] <command> [args...]\n", prog_name);
    printf("  sudo %s stats <PID>                  Display live container resource metrics\n", prog_name);
    printf("Options:\n");
    printf("  --rootfs <dir>     Path to rootfs directory (default: 'rootfs')\n");
    printf("  --memory <limit>   Memory limit in bytes or string (e.g. '50M' or '52428800')\n");
    printf("  --cpu <quota>      CPU quota specification (e.g. '20000 100000')\n");
    printf("  --pids <limit>     Maximum PIDs/threads allowed (e.g. 20)\n");
    printf("  --net              Enable network namespace isolation and veth pair setup\n");
    printf("  %s version         Display version info\n", prog_name);
    printf("  %s help            Show this help menu\n", prog_name);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "help") == 0 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        print_usage(argv[0]);
        return 0;
    }

    if (strcmp(argv[1], "version") == 0 || strcmp(argv[1], "-v") == 0) {
        printf("mydocker version 0.6.0 (Stage 6: Seccomp BPF Sandbox & Stats Monitor)\n");
        return 0;
    }

    if (strcmp(argv[1], "stats") == 0) {
        if (argc < 3) {
            LOG_ERROR("Usage: %s stats <PID>", argv[0]);
            return 1;
        }
        pid_t target_pid = (pid_t)atoi(argv[2]);
        return cgroups_print_stats(target_pid);
    }

    if (strcmp(argv[1], "run") == 0) {
        if (argc < 3) {
            LOG_ERROR("No command specified for 'run'. Usage: %s run [OPTIONS] <command> [args...]", argv[0]);
            return 1;
        }

        int arg_idx = 2;
        char *rootfs_dir = NULL;
        char *mem_max = NULL;
        char *cpu_max = NULL;
        int pids_max = 0;
        int enable_net = 0;

        // Parse optional CLI flags
        while (arg_idx < argc && argv[arg_idx][0] == '-') {
            if (strcmp(argv[arg_idx], "--rootfs") == 0) {
                if (arg_idx + 1 >= argc) {
                    LOG_ERROR("--rootfs requires a directory path.");
                    return 1;
                }
                rootfs_dir = argv[arg_idx + 1];
                arg_idx += 2;
            } else if (strcmp(argv[arg_idx], "--memory") == 0) {
                if (arg_idx + 1 >= argc) {
                    LOG_ERROR("--memory requires a limit argument.");
                    return 1;
                }
                mem_max = argv[arg_idx + 1];
                arg_idx += 2;
            } else if (strcmp(argv[arg_idx], "--cpu") == 0) {
                if (arg_idx + 1 >= argc) {
                    LOG_ERROR("--cpu requires a quota argument.");
                    return 1;
                }
                cpu_max = argv[arg_idx + 1];
                arg_idx += 2;
            } else if (strcmp(argv[arg_idx], "--pids") == 0) {
                if (arg_idx + 1 >= argc) {
                    LOG_ERROR("--pids requires an integer limit.");
                    return 1;
                }
                pids_max = atoi(argv[arg_idx + 1]);
                arg_idx += 2;
            } else if (strcmp(argv[arg_idx], "--net") == 0) {
                enable_net = 1;
                arg_idx += 1;
            } else {
                break;
            }
        }

        if (arg_idx >= argc) {
            LOG_ERROR("No command specified to run inside container.");
            return 1;
        }

        // Default to "rootfs" directory if it exists and no --rootfs was specified
        if (!rootfs_dir) {
            if (access("rootfs/bin/sh", F_OK) == 0) {
                rootfs_dir = "rootfs";
            }
        }

        container_config_t config;
        memset(&config, 0, sizeof(config));
        config.hostname = "mydocker-container";
        config.rootfs = rootfs_dir;
        config.cgroups.mem_max = mem_max;
        config.cgroups.cpu_max = cpu_max;
        config.cgroups.pids_max = pids_max;
        config.net.enable_net = enable_net;
        config.argv = &argv[arg_idx];
        config.argc = argc - arg_idx;

        return container_run(&config);
    }

    LOG_ERROR("Unknown command: '%s'", argv[1]);
    print_usage(argv[0]);
    return 1;
}
