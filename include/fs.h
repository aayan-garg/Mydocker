#ifndef MYDOCKER_FS_H
#define MYDOCKER_FS_H

#include <sys/types.h>

typedef struct {
    char lower_dir[512];   // Base read-only rootfs (e.g. "rootfs")
    char upper_dir[512];   // Container read-write diff dir
    char work_dir[512];    // OverlayFS internal workdir
    char merged_dir[512];  // Combined OverlayFS view mount point
    int is_overlay_active;
} overlay_config_t;

// Setup OverlayFS storage layers and return path to merged rootfs
int fs_prepare_overlay(const char *lower_path, pid_t pid, overlay_config_t *ovl);

// Clean up and unmount OverlayFS layers
int fs_cleanup_overlay(overlay_config_t *ovl);

// Setup container rootfs jail using pivot_root and mount pseudo filesystems
int fs_setup_root(const char *rootfs_path);

#endif // MYDOCKER_FS_H
