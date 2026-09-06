#include <errno.h>
#include <seccomp.h>
#include <stdio.h>

#include "seccomp_filter.h"
#include "utils.h"

int seccomp_apply_filter(void) {
    LOG_INFO("Applying Seccomp BPF syscall filter security sandbox...");

    // Default action: ALLOW all syscalls except explicitly denied rules
    scmp_filter_ctx ctx = seccomp_init(SCMP_ACT_ALLOW);
    if (!ctx) {
        LOG_ERROR("Failed to initialize Seccomp filter context.");
        return -1;
    }

    // Block reboot() system call (returns EPERM)
    if (seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EPERM), SCMP_SYS(reboot), 0) != 0) {
        LOG_WARN("Failed to add Seccomp rule for reboot.");
    }

    // Block kexec_load() system call
    if (seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EPERM), SCMP_SYS(kexec_load), 0) != 0) {
        LOG_WARN("Failed to add Seccomp rule for kexec_load.");
    }

    // Block ptrace() system call (process tracing / debugging)
    if (seccomp_rule_add(ctx, SCMP_ACT_ERRNO(EPERM), SCMP_SYS(ptrace), 0) != 0) {
        LOG_WARN("Failed to add Seccomp rule for ptrace.");
    }

    // Load filter into kernel
    if (seccomp_load(ctx) != 0) {
        LOG_ERROR("Failed to load Seccomp BPF filter into kernel.");
        seccomp_release(ctx);
        return -1;
    }

    seccomp_release(ctx);
    LOG_INFO("Seccomp BPF filter active: reboot, kexec_load, and ptrace syscalls restricted.");
    return 0;
}
