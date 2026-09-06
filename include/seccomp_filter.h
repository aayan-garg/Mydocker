#ifndef MYDOCKER_SECCOMP_FILTER_H
#define MYDOCKER_SECCOMP_FILTER_H

// Apply Seccomp BPF syscall filter to restrict dangerous kernel calls (reboot, ptrace, kexec_load)
int seccomp_apply_filter(void);

#endif // MYDOCKER_SECCOMP_FILTER_H
