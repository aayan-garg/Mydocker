#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "cgroups.h"
#include "container.h"
#include "fs.h"
#include "net.h"
#include "seccomp_filter.h"
#include "utils.h"

typedef struct {
    container_config_t *config;
    const char *child_rootfs;
    int sync_pipe_read_fd;
} container_child_args_t;

static int container_child(void *arg) {
    container_child_args_t *args = (container_child_args_t *)arg;
    container_config_t *config = args->config;

    // 1. Wait for parent process to finish cgroups, netns & OverlayFS setup
    char sync_buf;
    if (read(args->sync_pipe_read_fd, &sync_buf, 1) != 1) {
        LOG_ERROR("Child failed synchronization read from parent.");
        close(args->sync_pipe_read_fd);
        return -1;
    }
    close(args->sync_pipe_read_fd);

    // 2. Set isolated hostname inside UTS namespace
    if (config->hostname && strlen(config->hostname) > 0) {
        if (sethostname(config->hostname, strlen(config->hostname)) != 0) {
            perror("sethostname failed");
            return -1;
        }
    }

    // 3. Setup rootfs jail if specified
    const char *target_rootfs = args->child_rootfs ? args->child_rootfs : config->rootfs;
    if (target_rootfs && strlen(target_rootfs) > 0) {
        if (fs_setup_root(target_rootfs) != 0) {
            LOG_ERROR("Failed to setup rootfs jail at '%s'", target_rootfs);
            return -1;
        }
    }

    // 4. Apply Seccomp BPF syscall filter security sandbox
    if (seccomp_apply_filter() != 0) {
        LOG_WARN("Continuing container execution without Seccomp BPF sandbox.");
    }

    // 5. Execute requested command
    LOG_CONTAINER("Executing process inside container: %s", config->argv[0]);
    execvp(config->argv[0], config->argv);
    
    // If execvp returns, an error occurred
    perror("execvp failed");
    return -1;
}

int container_run(container_config_t *config) {
    char *stack = malloc(CONTAINER_STACK_SIZE);
    if (!stack) {
        LOG_ERROR("Failed to allocate stack memory for container process.");
        return -1;
    }

    // Create synchronization pipe between parent and child
    int sync_pipe[2];
    if (pipe(sync_pipe) != 0) {
        perror("pipe failed");
        free(stack);
        return -1;
    }

    // Setup OverlayFS Copy-on-Write storage layer if rootfs path provided
    overlay_config_t ovl;
    memset(&ovl, 0, sizeof(ovl));

    const char *target_rootfs = config->rootfs;
    if (config->rootfs && strlen(config->rootfs) > 0) {
        pid_t temp_id = getpid();
        if (fs_prepare_overlay(config->rootfs, temp_id, &ovl) == 0) {
            target_rootfs = ovl.merged_dir;
        }
    }

    // Stack grows downward on x86_64, pass top of stack
    char *stack_top = stack + CONTAINER_STACK_SIZE;

    // Namespace flags: PID, UTS, Mount, and Network namespaces
    int clone_flags = CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWNS | SIGCHLD;
    if (config->net.enable_net) {
        clone_flags |= CLONE_NEWNET;
    }

    container_child_args_t child_args = {
        .config = config,
        .child_rootfs = target_rootfs,
        .sync_pipe_read_fd = sync_pipe[0]
    };

    bench_timer_t t;
    bench_timer_start(&t);

    LOG_INFO("Spawning container process with isolated PID, UTS, Mount%s namespaces...",
             config->net.enable_net ? ", and NET" : "");
    pid_t child_pid = clone(container_child, stack_top, clone_flags, &child_args);

    if (child_pid == -1) {
        perror("clone failed");
        close(sync_pipe[0]);
        close(sync_pipe[1]);
        if (ovl.is_overlay_active) fs_cleanup_overlay(&ovl);
        free(stack);
        return -1;
    }

    // Close read end of pipe in parent
    close(sync_pipe[0]);

    // Attach cgroups v2 limits to child process
    cgroups_setup(child_pid, &config->cgroups);

    // Setup virtual ethernet pair and network routing if enabled
    if (config->net.enable_net) {
        net_setup(child_pid, &config->net);
    }

    // Unblock child process by writing 1 byte to sync pipe
    if (write(sync_pipe[1], "1", 1) != 1) {
        perror("write sync_pipe failed");
    }
    close(sync_pipe[1]);

    double elapsed = bench_timer_elapsed_ms(&t);
    LOG_INFO("Container spawned & sandbox initialized (PID: %d) in %.2f ms", child_pid, elapsed);

    // Wait for container child process to exit
    int status = 0;
    if (waitpid(child_pid, &status, 0) == -1) {
        perror("waitpid failed");
    } else {
        if (WIFEXITED(status)) {
            LOG_INFO("Container process exited with status %d", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            LOG_WARN("Container process terminated by signal %d (%s)", WTERMSIG(status), strsignal(WTERMSIG(status)));
        }
    }

    if (config->net.enable_net) {
        net_cleanup(child_pid);
    }
    cgroups_cleanup(child_pid);
    if (ovl.is_overlay_active) {
        fs_cleanup_overlay(&ovl);
    }

    free(stack);
    return 0;
}
