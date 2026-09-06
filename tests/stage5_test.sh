#!/bin/bash
set -e

GREEN='\033[1;32m'
RED='\033[1;31m'
RESET='\033[0m'

echo "================================================="
echo "Running Stage 5 Verification: Network Sandbox & veth"
echo "================================================="

if [ "$(id -u)" -ne 0 ]; then
    echo -e "${RED}[FAIL] Stage 5 test must be run as root (sudo)${RESET}"
    exit 1
fi

BIN="./bin/mydocker"

if [ ! -f "$BIN" ]; then
    echo -e "${RED}[FAIL] Binary $BIN not found. Run make first.${RESET}"
    exit 1
fi

# Test 1: Container IP Assignment inside Network Namespace
echo "[TEST 1] Testing Container Interface (eth0) & IP Assignment..."
CONTAINER_IP=$($BIN run --net --rootfs rootfs /bin/sh -c "ip addr show eth0" | grep 'inet ' | awk '{print $2}' | cut -d'/' -f1 | tr -d '\r\n')

echo "Container IP: '$CONTAINER_IP'"

if [ "$CONTAINER_IP" != "172.19.0.2" ]; then
    echo -e "${RED}[FAIL] Expected container IP to be '172.19.0.2', got '$CONTAINER_IP'${RESET}"
    exit 1
fi
echo -e "${GREEN}[PASS] Network namespace & eth0 IP assignment (172.19.0.2) verified.${RESET}"

# Test 2: Host Gateway Connectivity Ping
echo "[TEST 2] Testing Container -> Host Gateway (172.19.0.1) Connectivity..."
set +e
PING_RES=$($BIN run --net --rootfs rootfs /bin/sh -c "ping -c 2 172.19.0.1" 2>&1)
set -e

echo "$PING_RES"

if echo "$PING_RES" | grep -q "0% packet loss"; then
    echo -e "${GREEN}[PASS] veth pair bridge connectivity verified (0% packet loss to host gateway).${RESET}"
else
    echo -e "${RED}[FAIL] Ping to host gateway failed.${RESET}"
    exit 1
fi

echo "================================================="
echo -e "${GREEN}Stage 5 Verification Passed Successfully!${RESET}"
echo "================================================="
