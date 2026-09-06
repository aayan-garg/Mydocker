# Minimal Linux Container Runtime (`mydocker`)

[![C Standard](https://img.shields.io/badge/C-gnu11-blue.svg)](https://en.wikipedia.org/wiki/C11_(C_standard_revision))
[![Kernel](https://img.shields.io/badge/Linux-6.6%2B-orange.svg)](https://kernel.org)
[![cgroups](https://img.shields.io/badge/cgroups-v2_unified-green.svg)](https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v2.html)
[![License](https://img.shields.io/badge/License-MIT-brightgreen.svg)](LICENSE)

`mydocker` is a zero-dependency, lightweight Linux container runtime engineered in **C**. It builds container execution sandboxes directly from native Linux kernel primitives—without relying on Docker, `containerd`, or `runc`.

---

## Technical Architecture

```
+-----------------------------------------------------------------------------------+
|                                Host Linux Kernel                                  |
+-----------------------------------------------------------------------------------+
                                         |
     +-----------------------------------+-----------------------------------+
     |                                   |                                   |
     v                                   v                                   v
+------------------------+   +-----------------------+   +--------------------------+
| Namespaces (NS)        |   | cgroups v2 Limits     |   | Storage & Security       |
| - PID  (PID 1 inside)  |   | - memory.max (RAM)    |   | - pivot_root (RootFS)    |
| - UTS  (Hostname)      |   | - pids.max (Threads)  |   | - OverlayFS (CoW)        |
| - Mount (Isolated FS)  |   | - cpu.max (Quota)     |   | - Seccomp BPF (Syscalls) |
| - Net  (veth Pair)     |   +-----------------------+   +--------------------------+
+------------------------+
```

---

## Core Features & Kernel Implementations

1. **Process & Hostname Isolation (Linux Namespaces)**
   - `CLONE_NEWPID`: Provides an isolated PID tree; container main process becomes `PID 1`.
   - `CLONE_NEWUTS`: Isolates hostname (`mydocker-container`) without mutating host system hostname.
   - `CLONE_NEWNS`: Mount propagation isolated via `MS_PRIVATE`.

2. **Filesystem Jail & Copy-on-Write Layering (`pivot_root` + OverlayFS)**
   - Swaps container root filesystem using `pivot_root` (preferred over `chroot` to prevent jailbreak exploits).
   - Mounts container-private `/proc` and `/sys` virtual filesystems.
   - **OverlayFS Storage Driver**: Merges read-only base rootfs (`lowerdir`) with container diff layer (`upperdir`), leaving the base Alpine image 100% read-only and unpolluted.

3. **Resource Bounds & Control Groups (cgroups v2)**
   - Automatically provisions cgroup nodes at `/sys/fs/cgroup/mydocker_<PID>`.
   - Enforces hard RAM limits (`memory.max`), process caps (`pids.max`), and CPU bandwidth quotas (`cpu.max`).
   - Uses IPC synchronization pipes to guarantee cgroups attachment prior to container child execution.

4. **Network Namespace Isolation & `veth` Pair Setup**
   - Spawns dedicated network stack via `CLONE_NEWNET`.
   - Dynamically configures virtual ethernet (`veth`) interface pairs, assigning container IP `172.19.0.2/24` and routing traffic through host gateway `172.19.0.1`.

5. **Security Hardening (Seccomp BPF)**
   - Restricts high-risk kernel system calls (`reboot`, `ptrace`, `kexec_load`) via Berkeley Packet Filters (BPF).

6. **Live Container Resource Monitor (`mydocker stats`)**
   - Interrogates cgroups v2 counters to display real-time memory RSS, active thread count, and limit utilization.

---

## Repository Structure

```
Mydocker/
├── Makefile                   # Build system (compile, setup-rootfs, test)
├── README.md                  # Project documentation & resume guide
├── PROJECT_TRACKER.md         # Milestones, activity log & stage benchmarks
├── include/                   # C Header files
│   ├── cgroups.h              # cgroups v2 controller API
│   ├── container.h            # Container configuration & clone wrapper
│   ├── fs.h                   # RootFS, pivot_root & OverlayFS helpers
│   ├── net.h                  # Virtual ethernet bridge API
│   ├── seccomp_filter.h       # Seccomp BPF syscall filter
│   └── utils.h                # Logging macros & high-precision timers
├── src/                       # C Implementation source code
│   ├── main.c                 # CLI launcher (run / stats / version / help)
│   ├── container.c            # Process cloning & IPC synchronization
│   ├── cgroups.c              # cgroups v2 limit configuration & metrics reader
│   ├── fs.c                   # Mount propagation, pivot_root & OverlayFS mount
│   ├── net.c                  # Network namespace & veth bridge configuration
│   ├── seccomp_filter.c       # Seccomp BPF rules loader
│   ├── mem_alloc.c            # Memory stress test tool
│   ├── reboot_test.c          # Syscall security test tool
│   └── utils.c                # High-resolution monotonic timers
├── rootfs/                    # Alpine Linux minirootfs base image
└── tests/                     # Verification test suite scripts
    ├── stage1_test.sh         # Namespace isolation tests
    ├── stage2_test.sh         # RootFS jail & /proc isolation tests
    ├── stage3_test.sh         # cgroups v2 resource limit tests
    ├── stage4_test.sh         # OverlayFS CoW isolation tests
    ├── stage5_test.sh         # Network namespace & veth ping tests
    └── stage6_test.sh         # Seccomp EPERM & mydocker stats tests
```

---

## Build & Usage Guide (WSL2 / Linux)

### Prerequisites (WSL2 / Ubuntu)
```bash
sudo apt-get update
sudo apt-get install -y gcc make libseccomp-dev
```

### 1. Build Runtime & Setup RootFS
```bash
# Compile binary
make

# Download & extract Alpine Linux base rootfs
sudo make setup-rootfs
```

### 2. Run Container CLI Examples

```bash
# Run interactive shell inside container
sudo ./bin/mydocker run --rootfs rootfs /bin/sh

# Run with Memory, Process limits & Network sandbox
sudo ./bin/mydocker run --memory 50M --pids 20 --net --rootfs rootfs /bin/sh -c "ip addr show eth0; hostname"

# Inspect live metrics for active container PID
sudo ./bin/mydocker stats <PID>
```

### 3. Execute Automated Verification Test Suite
```bash
sudo make test
```

---

## Resume Showcase

**Minimal Linux Container Runtime (`mydocker`)** | *C, Linux Syscalls, cgroups v2, POSIX, OverlayFS, Seccomp*
- Engineered a zero-dependency container runtime in C using native Linux kernel system calls (`clone`, `pivot_root`), achieving sub-20ms process isolation and rootfs jailing.
- Implemented **cgroups v2** controllers to enforce memory bounds (`memory.max`), CPU quotas (`cpu.max`), and process limits (`pids.max`), preventing fork-bomb exploits and noisy-neighbor memory spikes.
- Designed an **OverlayFS Copy-on-Write (CoW)** storage driver to merge read-only base images with ephemeral container write layers, preserving base image integrity across runs.
- Built a **Seccomp BPF** syscall security filter blocking dangerous kernel calls (`sys_reboot`, `ptrace`), and created a built-in `mydocker stats` monitor for real-time memory and task metrics.

---

## License
Licensed under the [MIT License](LICENSE).
