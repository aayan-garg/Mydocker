#ifndef MYDOCKER_NET_H
#define MYDOCKER_NET_H

#include <sys/types.h>

typedef struct {
    int enable_net;      // 1 to enable network isolation and veth pair setup
    char host_ip[32];    // Host gateway IP (e.g. "172.19.0.1/24")
    char container_ip[32]; // Container IP (e.g. "172.19.0.2/24")
} net_config_t;

// Setup virtual ethernet pair, assign IPs, and attach container netns
int net_setup(pid_t child_pid, net_config_t *net_cfg);

// Clean up veth interface on host
int net_cleanup(pid_t child_pid);

#endif // MYDOCKER_NET_H
