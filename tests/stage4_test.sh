#!/bin/bash
set -e

GREEN='\033[1;32m'
RED='\033[1;31m'
RESET='\033[0m'

echo "================================================="
echo "Running Stage 4 Verification: OverlayFS CoW"
echo "================================================="

if [ "$(id -u)" -ne 0 ]; then
    echo -e "${RED}[FAIL] Stage 4 test must be run as root (sudo)${RESET}"
    exit 1
fi

BIN="./bin/mydocker"

if [ ! -f "$BIN" ]; then
    echo -e "${RED}[FAIL] Binary $BIN not found. Run make first.${RESET}"
    exit 1
fi

# Ensure base rootfs test file doesn't exist initially
rm -f rootfs/test_cow_file.txt

# Test 1: Write file inside container
echo "[TEST 1] Writing file inside container OverlayFS layer..."
$BIN run --rootfs rootfs /bin/sh -c "echo 'container_data_123' > /test_cow_file.txt; cat /test_cow_file.txt"

# Test 2: Verify base image remains unpolluted
echo "[TEST 2] Verifying base image 'rootfs/' unpolluted..."
if [ -f "rootfs/test_cow_file.txt" ]; then
    echo -e "${RED}[FAIL] Base image rootfs/test_cow_file.txt was mutated! OverlayFS CoW failed.${RESET}"
    exit 1
fi
echo -e "${GREEN}[PASS] Base image rootfs/ remained 100% clean and unpolluted.${RESET}"

# Test 3: Verify fresh container run does not see previous container writes
echo "[TEST 3] Verifying fresh container launch clean state..."
set +e
MISSING_CHECK=$($BIN run --rootfs rootfs /bin/sh -c "cat /test_cow_file.txt" 2>&1)
set -e

if echo "$MISSING_CHECK" | grep -q "No such file or directory"; then
    echo -e "${GREEN}[PASS] OverlayFS CoW verified! Container write layers are isolated and cleaned up.${RESET}"
else
    echo -e "${RED}[FAIL] Fresh container retained file from previous run.${RESET}"
    exit 1
fi

echo "================================================="
echo -e "${GREEN}Stage 4 Verification Passed Successfully!${RESET}"
echo "================================================="
