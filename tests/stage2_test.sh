#!/bin/bash
set -e

GREEN='\033[1;32m'
RED='\033[1;31m'
RESET='\033[0m'

echo "================================================="
echo "Running Stage 2 Verification: RootFS Jail & /proc"
echo "================================================="

if [ "$(id -u)" -ne 0 ]; then
    echo -e "${RED}[FAIL] Stage 2 test must be run as root (sudo)${RESET}"
    exit 1
fi

BIN="./bin/mydocker"

if [ ! -f "$BIN" ]; then
    echo -e "${RED}[FAIL] Binary $BIN not found. Run make first.${RESET}"
    exit 1
fi

if [ ! -d "rootfs" ]; then
    echo -e "${RED}[FAIL] rootfs directory not found. Run make setup-rootfs first.${RESET}"
    exit 1
fi

# Test 1: RootFS Jail Verification (Alpine Linux OS Release)
echo "[TEST 1] Testing pivot_root RootFS Jail..."
OS_NAME=$($BIN run --rootfs rootfs /bin/sh -c "cat /etc/os-release" | grep '^NAME=' | cut -d'=' -f2 | tr -d '"\r\n')

echo "Container OS Name: '$OS_NAME'"

if [[ "$OS_NAME" != *"Alpine Linux"* ]]; then
    echo -e "${RED}[FAIL] Expected container OS to be Alpine Linux, got '$OS_NAME'${RESET}"
    exit 1
fi
echo -e "${GREEN}[PASS] pivot_root RootFS jail verified (Alpine Linux running inside jail).${RESET}"

# Test 2: Isolated /proc Mount Verification
echo "[TEST 2] Testing Isolated /proc Mount Process Visibility..."
PROCESS_COUNT=$($BIN run --rootfs rootfs /bin/sh -c "ps aux" | grep -v '\[INFO\]' | grep -v '\[CONTAINER\]' | grep -v '^PID' | wc -l | tr -d ' ')

echo "Total visible processes inside container: $PROCESS_COUNT"

if [ "$PROCESS_COUNT" -gt 5 ]; then
    echo -e "${RED}[FAIL] Container sees $PROCESS_COUNT processes! Host processes are leaking into container /proc.${RESET}"
    exit 1
fi
echo -e "${GREEN}[PASS] Isolated /proc mount verified (Only $PROCESS_COUNT container processes visible).${RESET}"

echo "================================================="
echo -e "${GREEN}Stage 2 Verification Passed Successfully!${RESET}"
echo "================================================="
