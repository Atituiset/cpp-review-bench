// AUTO-DRAFT from redis/redis PR #15518
void clusterSendFailoverAuthIfNeeded(clusterNode *node, clusterMsg *request);
void clusterUpdateState(void);
static void clusterFireTopologyChangeEventIfNeeded(void);
static void clusterNotifyTopologyChange(uint64_t change_flags);  // <<< BUG ANCHOR
int clusterNodeCoversSlot(clusterNode *n, int slot);
list *clusterGetNodesInMyShard(clusterNode *node);
int clusterNodeAddSlave(clusterNode *master, clusterNode *slave);
* The option can be set at runtime via CONFIG SET. */
void clusterUpdateMyselfAnnouncedPorts(void) {
    if (!myself) return;
    deriveAnnouncedPorts(&myself->tcp_port,&myself->tls_port,&myself->cport);
}

/* We want to take myself->ip in sync with the cluster-announce-ip option.
        } else {
            myself->ip[0] = '\0'; /* Force autodetection. */
        }
    }
}

        sdsclear(node->hostname);
    }
    clusterDoBeforeSleep(CLUSTER_TODO_SAVE_CONFIG);
}

static void updateAnnouncedHumanNodename(clusterNode *node, char *new) {
    if (node->tcp_port == tcp_port && node->cport == cport && node->tls_port == tls_port &&
        strcmp(ip,node->ip) == 0) return 0;

    /* IP / port is different, update it. */
    memcpy(node->ip,ip,sizeof(ip));
    node->tcp_port = tcp_port;
    if (nodeIsSlave(myself) && myself->slaveof == node)
        replicationSetMaster(node->ip, getNodeDefaultReplicationPort(node));

    /* A node moving to a different address is a topology change. */
    clusterNotifyTopologyChange(REDISMODULE_CLUSTER_TOPOLOGY_CHANGE_FLAG_NODE);
    return 1;
}


/* Record a pending RedisModuleEvent_ClusterTopologyChange (the change_flags are
 * REDISMODULE_CLUSTER_TOPOLOGY_CHANGE_FLAG_* bits) and request it to be fired
 * from the next clusterBeforeSleep(). Called from every slot/role mutation and
 * on the cluster's OK/FAIL transition. */
static void clusterNotifyTopologyChange(uint64_t change_flags) {
    server.cluster->topology_change_flags |= change_flags;
    clusterDoBeforeSleep(CLUSTER_TODO_FIRE_TOPOLOGY_CHANGE);
}
