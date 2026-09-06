# Minimal Linux Container Runtime (`mydocker`)

[![C Standard](https://img.shields.io/badge/C-gnu11-blue.svg)](https://en.wikipedia.org/wiki/C11_(C_standard_revision))
[![Kernel](https://img.shields.io/badge/Linux-6.6%2B-orange.svg)](https://kernel.org)
[![cgroups](https://img.shields.io/badge/cgroups-v2_unified-green.svg)](https://www.kernel.org/doc/html/latest/admin-guide/cgroup-v2.html)
[![License](https://img.shields.io/badge/License-MIT-brightgreen.svg)](LICENSE)

`mydocker` is a zero-dependency, lightweight Linux container runtime written in **C**. It provisions isolated process execution sandboxes directly using native Linux Kernel primitives—operating without external dependencies like Docker Engine, `containerd`, or `runc`.

---

## Architectural Overview

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

## Core Capabilities & Linux Kernel Implementation

### 1. Process & Hostname Isolation (Linux Namespaces)
- **`CLONE_NEWPID`**: Establishes a dedicated PID hierarchy; the container's entry process becomes `PID 1`.
- **`CLONE_NEWUTS`**: Isolates hostname and NIS domain settings (`mydocker-container`).
- **`CLONE_NEWNS`**: Enforces isolated mount tables using `MS_PRIVATE` propagation.

### 2. Inter-Process Synchronization Pipeline
- Employs an IPC pipe barrier (`sync_pipe`) between parent and cloned child processes to guarantee cgroups initialization, network allocation, and security filter loading prior to container code execution.

### 3. Filesystem Jailing & Copy-on-Write Layering (`pivot_root` + OverlayFS)
- Swaps container root filesystem using `pivot_root` and detaches the host root (`umount2("/.old_root", MNT_DETACH)`).
- Provisions container-private `/proc`, `/sys`, and `/dev` virtual pseudo-filesystems.
- **OverlayFS Storage Driver**: Merges read-only base rootfs (`lowerdir`) with ephemeral container write layers (`upperdir`), preserving base image immutability across runs.

### 4. Resource Allocation & Hard Limits (cgroups v2)
- Provisions cgroup nodes at `/sys/fs/cgroup/mydocker_<PID>`.
- Enforces hard RAM limits (`memory.max`), disables swap spillover (`memory.swap.max = 0`), caps thread/process counts (`pids.max`), and configures CPU bandwidth quotas (`cpu.max`).

### 5. Network Isolation & `veth` Pair Setup
- Spawns isolated network stacks (`CLONE_NEWNET`).
- Configures virtual ethernet (`veth`) interface pairs, assigning container IP (`172.19.0.2/24`) and default routing via host gateway (`172.19.0.1`).

### 6. Security Hardening (Seccomp BPF)
- Loads Berkeley Packet Filters (BPF) via `libseccomp` to restrict high-risk system calls (`reboot`, `ptrace`, `kexec_load`).

### 7. Real-Time Container Metrics Monitor (`mydocker stats`)
- Reads cgroups v2 counters to render live RSS memory usage, active task counts, and resource limit utilization.

---

## Repository Structure

```
Mydocker/
├── Makefile                   # Build system (compile, setup-rootfs, test, clean)
├── README.md                  # Comprehensive technical documentation
├── PROJECT_TRACKER.md         # Milestone activity log & stage benchmarks
├── include/                   # Header definitions
│   ├── cgroups.h              # cgroups v2 controller interface
│   ├── container.h            # Container configuration & process lifecycle
│   ├── fs.h                   # RootFS, pivot_root & OverlayFS helpers
│   ├── net.h                  # Virtual ethernet bridge API
│   ├── seccomp_filter.h       # Seccomp BPF filter definitions
│   └── utils.h                # Logging macros & high-precision timers
├── src/                       # Source implementation
│   ├── main.c                 # CLI interface launcher
│   ├── container.c            # Process cloning & IPC synchronization
│   ├── cgroups.c              # cgroups v2 limit configuration & stats parser
│   ├── fs.c                   # Mount propagation, pivot_root & OverlayFS mount
│   ├── net.c                  # Network namespace & veth configuration
│   ├── seccomp_filter.c       # Seccomp BPF filter initialization
│   ├── mem_alloc.c            # Memory stress testing utility
│   ├── reboot_test.c          # Syscall security test tool
│   └── utils.c                # High-resolution monotonic timers
├── rootfs/                    # Alpine Linux minirootfs base directory
└── tests/                     # Automated test suites
    ├── stage1_test.sh         # Namespace isolation tests
    ├── stage2_test.sh         # RootFS jail & /proc isolation tests
    ├── stage3_test.sh         # cgroups v2 resource limit tests
    ├── stage4_test.sh         # OverlayFS CoW isolation tests
    ├── stage5_test.sh         # Network namespace & veth ping tests
    └── stage6_test.sh         # Seccomp EPERM & mydocker stats tests
```

---

## Getting Started

### System Requirements
- Linux Environment (Ubuntu 22.04+, Debian 12+, or WSL2)
- GCC Compiler (`gcc 11+`)
- Make & GNU Coreutils
- `libseccomp-dev` library

```bash
# Ubuntu / Debian / WSL2
sudo apt-get update
sudo apt-get install -y gcc make libseccomp-dev
```

### Build & Setup

```bash
# 1. Clone repository
git clone https://github.com/aayan-garg/Mydocker.git
cd Mydocker

# 2. Compile mydocker runtime
make

# 3. Download & prepare Alpine Linux base rootfs
sudo make setup-rootfs
```

---

## Command Line Interface (CLI) Reference

### 1. Run Container (`run`)

```bash
sudo ./bin/mydocker run [OPTIONS] <command> [args...]
```

**Supported Options:**
- `--rootfs <dir>`: Path to base rootfs directory (default: `rootfs`).
- `--memory <limit>`: Memory limit in bytes or formatted string (e.g. `50M` or `52428800`).
- `--cpu <quota>`: CPU quota specification formatted as `QUOTA PERIOD` (e.g. `20000 100000` for 20% CPU).
- `--pids <limit>`: Maximum thread/process limit (e.g. `20`).
- `--net`: Enables network namespace isolation and `veth` pair bridging.

#### CLI Examples

```bash
# Launch interactive shell inside rootfs jail
sudo ./bin/mydocker run --rootfs rootfs /bin/sh

# Launch container with 50MB RAM cap, 20 PIDs cap, and network isolation
sudo ./bin/mydocker run --memory 50M --pids 20 --net --rootfs rootfs /bin/sh -c "ip addr show eth0; hostname"
```

### 2. Live Container Stats (`stats`)

```bash
sudo ./bin/mydocker stats <PID>
```

#### Sample Output

```
================================================================
          CONTAINER REAL-TIME METRICS (mydocker stats)          
================================================================
 CONTAINER PID : 1036
 MEMORY USAGE  : 1.11 MB (1167360 bytes) / Limit: 67108864
 ACTIVE TASKS  : 1 active processes / Limit: 20
================================================================
```

---

## Automated Verification Suite

Run the full automated test suite verifying all 6 kernel isolation stages:

```bash
sudo make test
```

---

## License

This project is licensed under the [MIT License](LICENSE).
