#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "net.h"
#include "utils.h"

static int run_cmd(const char *cmd) {
    int res = system(cmd);
    if (res != 0) {
        LOG_WARN("Network configuration command failed: '%s'", cmd);
    }
    return res;
}

int net_setup(pid_t child_pid, net_config_t *net_cfg) {
    if (!net_cfg || !net_cfg->enable_net) return 0;

    char host_veth[32], child_veth[32];
    snprintf(host_veth, sizeof(host_veth), "veth_h%d", child_pid);
    snprintf(child_veth, sizeof(child_veth), "veth_c%d", child_pid);

    LOG_INFO("Setting up virtual ethernet pair ('%s' <-> '%s') for PID %d...", host_veth, child_veth, child_pid);

    char cmd[512];

    // 1. Create veth pair
    snprintf(cmd, sizeof(cmd), "ip link add %s type veth peer name %s", host_veth, child_veth);
    if (run_cmd(cmd) != 0) return -1;

    // 2. Move child veth into container network namespace
    snprintf(cmd, sizeof(cmd), "ip link set %s netns %d", child_veth, child_pid);
    if (run_cmd(cmd) != 0) return -1;

    // 3. Configure host veth interface IP and bring UP
    const char *host_ip = (strlen(net_cfg->host_ip) > 0) ? net_cfg->host_ip : "172.19.0.1/24";
    snprintf(cmd, sizeof(cmd), "ip addr add %s dev %s 2>/dev/null || true", host_ip, host_veth);
    run_cmd(cmd);

    snprintf(cmd, sizeof(cmd), "ip link set %s up", host_veth);
    run_cmd(cmd);

    // 4. Configure container interfaces via nsenter
    const char *container_ip = (strlen(net_cfg->container_ip) > 0) ? net_cfg->container_ip : "172.19.0.2/24";
    
    snprintf(cmd, sizeof(cmd), "nsenter -t %d -n ip link set dev %s name eth0 2>/dev/null || true", child_pid, child_veth);
    run_cmd(cmd);

    snprintf(cmd, sizeof(cmd), "nsenter -t %d -n ip addr add %s dev eth0 2>/dev/null || true", child_pid, container_ip);
    run_cmd(cmd);

    snprintf(cmd, sizeof(cmd), "nsenter -t %d -n ip link set dev lo up", child_pid);
    run_cmd(cmd);

    snprintf(cmd, sizeof(cmd), "nsenter -t %d -n ip link set dev eth0 up", child_pid);
    run_cmd(cmd);

    snprintf(cmd, sizeof(cmd), "nsenter -t %d -n ip route add default via 172.19.0.1 2>/dev/null || true", child_pid);
    run_cmd(cmd);

    LOG_INFO("Network sandbox initialized. Host IP: 172.19.0.1 | Container IP: %s", container_ip);
    return 0;
}

int net_cleanup(pid_t child_pid) {
    char host_veth[32];
    snprintf(host_veth, sizeof(host_veth), "veth_h%d", child_pid);

    char cmd[256];
    snprintf(cmd, sizeof(cmd), "ip link delete %s 2>/dev/null || true", host_veth);
    return system(cmd);
}
