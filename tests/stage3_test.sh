#!/bin/bash
set -e

GREEN='\033[1;32m'
RED='\033[1;31m'
RESET='\033[0m'

echo "================================================="
echo "Running Stage 3 Verification: cgroups v2 Limits"
echo "================================================="

if [ "$(id -u)" -ne 0 ]; then
    echo -e "${RED}[FAIL] Stage 3 test must be run as root (sudo)${RESET}"
    exit 1
fi

BIN="./bin/mydocker"

if [ ! -f "$BIN" ]; then
    echo -e "${RED}[FAIL] Binary $BIN not found. Run make first.${RESET}"
    exit 1
fi

# Test 1: PIDs Limit Verification (Process Cap)
echo "[TEST 1] Testing PIDs Limit (Cap at 2 processes)..."
set +e
PID_OUTPUT=$($BIN run --pids 2 --rootfs rootfs /bin/sh -c "for i in 1 2 3 4 5; do (sleep 1 &); done; wait" 2>&1)
set -e

echo "$PID_OUTPUT"

if echo "$PID_OUTPUT" | grep -qE "(can't fork|Resource temporarily unavailable)"; then
    echo -e "${GREEN}[PASS] cgroups v2 PIDs Limit verified! Fork limit blocked excess process creation.${RESET}"
else
    echo -e "${RED}[FAIL] PIDs limit was not enforced properly.${RESET}"
    exit 1
fi

# Test 2: cgroups v2 Memory & CPU Configuration
echo "[TEST 2] Testing cgroups v2 Memory & CPU Limits Attachment..."
OUTPUT=$($BIN run --memory 50M --cpu "20000 100000" --pids 10 --rootfs rootfs /bin/sh -c "echo 'cgroups test active'" 2>&1)

echo "$OUTPUT"

if echo "$OUTPUT" | grep -q "Configured Memory limit" && echo "$OUTPUT" | grep -q "Configured CPU limit"; then
    echo -e "${GREEN}[PASS] cgroups v2 Memory & CPU limits successfully applied to container cgroup.${RESET}"
else
    echo -e "${RED}[FAIL] cgroups configuration failed.${RESET}"
    exit 1
fi

echo "================================================="
echo -e "${GREEN}Stage 3 Verification Passed Successfully!${RESET}"
echo "================================================="
