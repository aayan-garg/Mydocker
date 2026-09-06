#!/bin/bash
set -e

GREEN='\033[1;32m'
RED='\033[1;31m'
RESET='\033[0m'

echo "================================================="
echo "Running Stage 6 Verification: Seccomp & Stats"
echo "================================================="

if [ "$(id -u)" -ne 0 ]; then
    echo -e "${RED}[FAIL] Stage 6 test must be run as root (sudo)${RESET}"
    exit 1
fi

BIN="./bin/mydocker"

if [ ! -f "$BIN" ]; then
    echo -e "${RED}[FAIL] Binary $BIN not found. Run make first.${RESET}"
    exit 1
fi

# Test 1: Seccomp BPF Syscall Filtering (Blocked reboot call)
echo "[TEST 1] Testing Seccomp BPF Syscall Hardening (reboot call)..."
set +e
SECCOMP_OUTPUT=$($BIN run --rootfs rootfs /bin/sh -c "/bin/reboot_test" 2>&1)
EXIT_CODE=$?
set -e

echo "$SECCOMP_OUTPUT"

if echo "$SECCOMP_OUTPUT" | grep -q "SECCOMP_DENIED_EPERM"; then
    echo -e "${GREEN}[PASS] Seccomp BPF Syscall Sandbox verified! Kernel blocked reboot call with EPERM.${RESET}"
else
    echo -e "${RED}[FAIL] Seccomp filter did not block reboot call.${RESET}"
    exit 1
fi

# Test 2: Live Container Metrics Display (mydocker stats)
echo "[TEST 2] Testing Container Metrics Monitor (mydocker stats)..."

# Spawn a long-running container process in background
$BIN run --memory 64M --pids 20 --rootfs rootfs /bin/sh -c "sleep 4" &
CONTAINER_PARENT_PID=$!
sleep 1

# Find active container child PID from cgroup directory
CHILD_CGROUP=$(ls -d /sys/fs/cgroup/mydocker_* 2>/dev/null | head -n 1)

if [ -n "$CHILD_CGROUP" ]; then
    CHILD_PID=$(basename "$CHILD_CGROUP" | cut -d'_' -f2)
    echo "Found active container cgroup PID: $CHILD_PID"
    
    STATS_OUTPUT=$($BIN stats "$CHILD_PID" 2>&1)
    echo "$STATS_OUTPUT"
    
    if echo "$STATS_OUTPUT" | grep -q "CONTAINER REAL-TIME METRICS" && echo "$STATS_OUTPUT" | grep -q "MEMORY USAGE"; then
        echo -e "${GREEN}[PASS] Container metrics stats monitor verified.${RESET}"
    else
        echo -e "${RED}[FAIL] mydocker stats output invalid.${RESET}"
        exit 1
    fi
else
    echo -e "${RED}[FAIL] Active container cgroup not found for stats test.${RESET}"
    exit 1
fi

wait $CONTAINER_PARENT_PID 2>/dev/null || true

echo "================================================="
echo -e "${GREEN}Stage 6 Verification Passed Successfully!${RESET}"
echo "================================================="
