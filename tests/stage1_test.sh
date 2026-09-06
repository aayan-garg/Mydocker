#!/bin/bash
set -e

GREEN='\033[1;32m'
RED='\033[1;31m'
RESET='\033[0m'

echo "================================================="
echo "Running Stage 1 Verification: Namespace Isolation"
echo "================================================="

if [ "$(id -u)" -ne 0 ]; then
    echo -e "${RED}[FAIL] Stage 1 test must be run as root (sudo)${RESET}"
    exit 1
fi

BIN="./bin/mydocker"

if [ ! -f "$BIN" ]; then
    echo -e "${RED}[FAIL] Binary $BIN not found. Run make first.${RESET}"
    exit 1
fi

# Test 1: UTS Namespace Hostname Isolation
echo "[TEST 1] Testing UTS Namespace Hostname Isolation..."
HOST_HOSTNAME=$(hostname)
CONTAINER_HOSTNAME=$($BIN run hostname | grep -v '\[INFO\]' | grep -v '\[CONTAINER\]' | tr -d '\r\n')

echo "Host Hostname:      $HOST_HOSTNAME"
echo "Container Hostname: $CONTAINER_HOSTNAME"

if [ "$CONTAINER_HOSTNAME" != "mydocker-container" ]; then
    echo -e "${RED}[FAIL] Container hostname was expected to be 'mydocker-container', got '$CONTAINER_HOSTNAME'${RESET}"
    exit 1
fi

CURRENT_HOST_HOSTNAME=$(hostname)
if [ "$CURRENT_HOST_HOSTNAME" != "$HOST_HOSTNAME" ]; then
    echo -e "${RED}[FAIL] Container modified the host hostname! UTS isolation broken.${RESET}"
    exit 1
fi
echo -e "${GREEN}[PASS] UTS Namespace isolation verified.${RESET}"

# Test 2: PID Namespace Isolation (Container PID 1)
echo "[TEST 2] Testing PID Namespace Isolation..."
CONTAINER_PID=$($BIN run /bin/sh -c "echo \$\$" | grep -v '\[INFO\]' | grep -v '\[CONTAINER\]' | tr -d '\r\n')

echo "Container Root Process PID: $CONTAINER_PID"

if [ "$CONTAINER_PID" != "1" ]; then
    echo -e "${RED}[FAIL] Container PID inside PID namespace was expected to be 1, got '$CONTAINER_PID'${RESET}"
    exit 1
fi
echo -e "${GREEN}[PASS] PID Namespace isolation verified (Container PID = 1).${RESET}"

echo "================================================="
echo -e "${GREEN}Stage 1 Verification Passed Successfully!${RESET}"
echo "================================================="
