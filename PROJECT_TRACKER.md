# Minimal Linux Container Runtime (`mydocker`) - Project Tracker

## Project Overview
A lightweight, zero-dependency Linux Container Runtime built in C using native Linux Kernel features:
- **Namespaces**: Process (PID), Hostname (UTS), Mount (NS), Network (NET), User (USER)
- **Control Groups**: cgroups v2 resource limits (Memory, CPU, PIDs)
- **Filesystem**: RootFS `pivot_root`, private `/proc` & `/sys`, OverlayFS CoW
- **Security**: Seccomp BPF syscall filter
- **Monitoring**: Live metrics monitor (`mydocker stats`)

---

## Progress Checklist & Roadmap

- [x] **Stage 0: Project Setup & Environment Verification**
  - WSL2 Ubuntu 24.04, GCC 13.3.0, Linux Kernel 6.6, cgroups v2 unified mode confirmed.
  - Initial directory layout & Makefile created.
- [x] **Stage 1: Namespace Isolation & Process Lifecycle**
  - Implemented `clone()` with `CLONE_NEWPID`, `CLONE_NEWUTS`, `CLONE_NEWNS`.
  - Process PID inside container confirmed as `PID 1`.
  - Hostname isolated to `mydocker-container` in <10ms.
- [x] **Stage 2: RootFS Jail & Filesystem Isolation (`pivot_root`, `/proc`)**
  - Downloaded Alpine Linux minirootfs base.
  - Implemented `pivot_root()` system call with private mount propagation (`MS_PRIVATE`).
  - Mounted isolated `/proc` and `/sys` pseudo-filesystems inside container root.
  - Verified container sees ONLY container processes (`ps aux` count = 1).
- [x] **Stage 3: cgroups v2 Resource Bounds (Memory, CPU, PIDs)**
  - Implemented cgroups v2 resource controller at `/sys/fs/cgroup/mydocker_<PID>`.
  - Added IPC pipe synchronization to guarantee cgroups attachment before container child execution.
  - Enforced PIDs limit (`pids.max`), memory cap (`memory.max`), and CPU quotas (`cpu.max`).
  - Verified kernel blocks fork-bombs with `can't fork: Resource temporarily unavailable`.
- [x] **Stage 4: OverlayFS Copy-on-Write (CoW) Layering**
  - Implemented OverlayFS storage driver with `lowerdir`, `upperdir`, `workdir`, and `merged` layers.
  - Base rootfs image remains 100% read-only and unpolluted across container executions.
  - Ephemeral container write layers are automatically created and unmounted/cleaned up on exit.
- [x] **Stage 5: Network Isolation & `veth` Pair Bridge**
  - Implemented `CLONE_NEWNET` network namespace isolation.
  - Dynamically created virtual ethernet (`veth`) interface pairs connecting host and container.
  - Assigned container IP (`172.19.0.2/24`) and configured default route via host gateway (`172.19.0.1`).
  - Verified 0% packet loss ping connectivity from container to host bridge.
- [ ] **Stage 6: Security Sandbox (Seccomp) & Container Metrics Monitor**

---

## Activity Log & Benchmarks

### Stage 5: Network Isolation & veth Pair Bridge
- **Date**: 2026-09-06
- **Status**: Completed & Verified
- **Benchmark**:
  - Network Sandbox Setup: `eth0` configured to `172.19.0.2/24` (PASS)
  - Host Bridge Gateway Ping: 0% packet loss to `172.19.0.1` (PASS)
  - veth Interface Cleanup: Deleted on container exit (PASS)

### Stage 4: OverlayFS Copy-on-Write (CoW) Layering
- **Date**: 2026-09-06
- **Status**: Completed & Verified
- **Benchmark**:
  - Layer setup & mount timing: ~19.4 ms
  - Read-Only Base Image Protection: `rootfs/` unpolluted after writes (PASS)
  - Ephemeral Layer Cleanup: Unmounted & purged on container exit (PASS)

### Stage 3: cgroups v2 Resource Bounds
- **Date**: 2026-09-06
- **Status**: Completed & Verified
- **Benchmark**:
  - Spawn & attachment timing: ~17-27 ms
  - Synchronization: Inter-process pipe sync guarantees zero race conditions (PASS)
  - PIDs limit enforcement: `pids.max` = 2 blocks excess forks (PASS)
  - cgroups v2 controllers: `memory.max`, `cpu.max`, `pids.max` correctly applied & cleaned up (PASS)

### Stage 2: RootFS Jail & Filesystem Isolation
- **Date**: 2026-09-06
- **Status**: Completed & Verified
- **Benchmark**:
  - RootFS OS: Alpine Linux v3.20 (PASS)
  - Process Visibility: 1 active process inside container `/proc` (PASS)
  - Mount propagation: Host mounts protected via `MS_PRIVATE` (PASS)

### Stage 1: Namespace Isolation
- **Date**: 2026-09-06
- **Status**: Completed & Verified
- **Benchmark**:
  - Spawn timing: ~8.5 ms
  - UTS Isolation: `mydocker-container` vs host `LAPTOP-9H0QSNOD` (PASS)
  - PID Isolation: Container main process `PID 1` (PASS)

### Stage 0: Setup & Scaffolding
- **Date**: 2026-09-06
- **Status**: Completed
- **Notes**: Environment verified. WSL2 kernel supports cgroups v2 unified filesystem at `/sys/fs/cgroup`.
