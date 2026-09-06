#include <errno.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

int main(void) {
    printf("Attempting direct SYS_reboot system call...\n");
    int res = syscall(SYS_reboot, 0xfee1dead, 672274793, 0x01234567, NULL);
    if (res == -1 && errno == EPERM) {
        printf("SECCOMP_DENIED_EPERM: Kernel blocked reboot syscall with Operation not permitted!\n");
        return 42; // Special exit code for test
    }
    printf("Syscall returned %d, errno: %d\n", res, errno);
    return 0;
}
