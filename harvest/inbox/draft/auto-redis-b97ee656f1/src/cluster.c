// AUTO-DRAFT from redis/redis PR #13729
sds auxTlsPortGetter(clusterNode *n, sds s);
int auxTlsPortPresent(clusterNode *n);
static void clusterBuildMessageHdr(clusterMsg *hdr, int type, size_t msglen);
  // <<< BUG ANCHOR
int getNodeDefaultClientPort(clusterNode *n) {
    return server.tls_cluster ? n->tls_port : n->tcp_port;
        return C_ERR;
    }
    memcpy(n->shard_id, value, CLUSTER_NAMELEN);
    /* if n already has replicas, make sure they all agree
     * on the shard id */
    for (int i = 0; i < n->numslaves; i++) {
        if (memcmp(n->slaves[i]->shard_id, n->shard_id, CLUSTER_NAMELEN) != 0) {
            return C_ERR;
        }
    }
    clusterAddNodeToShard(value, n);
    return C_OK;
                clusterAddNode(master);
            }
            /* shard_id can be absent if we are loading a nodes.conf generated
             * by an older version of Redis; we should follow the primary's
             * shard_id in this case */
            if (auxFieldHandlers[af_shard_id].isPresent(n) == 0) {
                memcpy(n->shard_id, master->shard_id, CLUSTER_NAMELEN);
                clusterAddNodeToShard(master->shard_id, n);
            } else if (clusterGetNodesInMyShard(master) != NULL &&
                       memcmp(master->shard_id, n->shard_id, CLUSTER_NAMELEN) != 0)
            {
                /* If the primary has been added to a shard, make sure this
                 * node has the same persisted shard id as the primary. */
                goto fmterr;
            }
            n->slaveof = master;
            clusterNodeAddSlave(master,n);
        } else if (auxFieldHandlers[af_shard_id].isPresent(n) == 0) {
    }
    /* Config sanity check */
    if (server.cluster->myself == NULL) goto fmterr;

    zfree(line);
    fclose(fp);
    clusterDoBeforeSleep(CLUSTER_TODO_SAVE_CONFIG);
}


static void updateShardId(clusterNode *node, const char *shard_id) {
    if (shard_id && memcmp(node->shard_id, shard_id, CLUSTER_NAMELEN) != 0) {
        clusterRemoveNodeFromShard(node);
        memcpy(node->shard
