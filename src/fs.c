#define _GNU_SOURCE
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "fs.h"
#include "utils.h"

static int pivot_root(const char *new_root, const char *put_old) {
    return syscall(SYS_pivot_root, new_root, put_old);
}

static int remove_dir_recursive(const char *path) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", path);
    return system(cmd);
}

int fs_prepare_overlay(const char *lower_path, pid_t pid, overlay_config_t *ovl) {
    if (!lower_path || !ovl) return -1;

    memset(ovl, 0, sizeof(overlay_config_t));

    char base_layer_dir[512];
    snprintf(base_layer_dir, sizeof(base_layer_dir), "/tmp/mydocker_%d_layers", pid);

    snprintf(ovl->lower_dir, sizeof(ovl->lower_dir), "%s", lower_path);
    snprintf(ovl->upper_dir, sizeof(ovl->upper_dir), "%s/upper", base_layer_dir);
    snprintf(ovl->work_dir, sizeof(ovl->work_dir), "%s/work", base_layer_dir);
    snprintf(ovl->merged_dir, sizeof(ovl->merged_dir), "%s/merged", base_layer_dir);

    LOG_INFO("Setting up OverlayFS Copy-on-Write (CoW) layers for PID %d...", pid);

    // Create upper, work, and merged directories
    mkdir(base_layer_dir, 0755);
    mkdir(ovl->upper_dir, 0755);
    mkdir(ovl->work_dir, 0755);
    mkdir(ovl->merged_dir, 0755);

    // Resolve absolute paths
    char abs_lower[1024], abs_upper[1024], abs_work[1024], abs_merged[1024];
    if (!realpath(ovl->lower_dir, abs_lower) ||
        !realpath(ovl->upper_dir, abs_upper) ||
        !realpath(ovl->work_dir, abs_work) ||
        !realpath(ovl->merged_dir, abs_merged)) {
        perror("realpath overlay paths failed");
        return -1;
    }

    char opts[2048];
    snprintf(opts, sizeof(opts), "lowerdir=%s,upperdir=%s,workdir=%s", abs_lower, abs_upper, abs_work);

    LOG_INFO("Mounting OverlayFS to '%s'...", abs_merged);
    if (mount("overlay", abs_merged, "overlay", 0, opts) != 0) {
        perror("mount overlay failed");
        return -1;
    }

    ovl->is_overlay_active = 1;
    snprintf(ovl->merged_dir, sizeof(ovl->merged_dir), "%s", abs_merged);
    return 0;
}

int fs_cleanup_overlay(overlay_config_t *ovl) {
    if (!ovl || !ovl->is_overlay_active) return 0;

    LOG_INFO("Cleaning up OverlayFS mount point at '%s'...", ovl->merged_dir);
    if (umount2(ovl->merged_dir, MNT_DETACH) != 0) {
        perror("umount overlay failed");
    }

    char base_layer_dir[512];
    snprintf(base_layer_dir, sizeof(base_layer_dir), "/tmp/mydocker_*_layers");
    remove_dir_recursive("/tmp/mydocker_*_layers");
    ovl->is_overlay_active = 0;
    return 0;
}

int fs_setup_root(const char *rootfs_path) {
    char abs_rootfs[1024];
    if (realpath(rootfs_path, abs_rootfs) == NULL) {
        perror("realpath rootfs failed");
        return -1;
    }

    LOG_INFO("Configuring RootFS jail at '%s'...", abs_rootfs);

    // 1. Ensure private mount propagation so container mounts don't pollute host
    if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) != 0) {
        perror("mount MS_PRIVATE failed");
        return -1;
    }

    // 2. Bind mount rootfs onto itself to guarantee it is a mount point for pivot_root
    if (mount(abs_rootfs, abs_rootfs, NULL, MS_BIND | MS_REC, NULL) != 0) {
        perror("bind mount rootfs failed");
        return -1;
    }

    // 3. Create temporary directory inside rootfs for old root
    char old_root_dir[1024];
    snprintf(old_root_dir, sizeof(old_root_dir), "%s/.old_root", abs_rootfs);
    mkdir(old_root_dir, 0700);

    // 4. Perform pivot_root
    if (pivot_root(abs_rootfs, old_root_dir) != 0) {
        perror("pivot_root failed");
        return -1;
    }

    // 5. Change current directory to new root
    if (chdir("/") != 0) {
        perror("chdir / failed");
        return -1;
    }

    // 6. Unmount old host root and remove temporary directory
    if (umount2("/.old_root", MNT_DETACH) != 0) {
        perror("umount old_root failed");
        return -1;
    }
    rmdir("/.old_root");

    // 7. Mount isolated pseudo-filesystems (/proc, /sys, /dev)
    mkdir("/proc", 0755);
    if (mount("proc", "/proc", "proc", MS_NODEV | MS_NOEXEC | MS_NOSUID, NULL) != 0) {
        perror("mount /proc failed");
        return -1;
    }

    mkdir("/sys", 0755);
    if (mount("sysfs", "/sys", "sysfs", MS_NODEV | MS_NOEXEC | MS_NOSUID, NULL) != 0) {
        LOG_WARN("mount /sys failed: %s", strerror(errno));
    }

    mkdir("/dev", 0755);
    if (mount("tmpfs", "/dev", "tmpfs", MS_NOSUID, "mode=755") != 0) {
        LOG_WARN("mount /dev failed: %s", strerror(errno));
    }

    LOG_INFO("RootFS jail and isolated /proc successfully initialized.");
    return 0;
}
