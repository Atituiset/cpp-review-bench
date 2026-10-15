// AUTO-DRAFT from redis/redis PR #13637
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
 * Copyright (c) 2009-Present, Redis Ltd.
 * All rights reserved.
 *
 * Licensed under your choice of the Redis Source Available License 2.0
 * (RSALv2) or the Server Side Public License v1 (SSPLv1).
 */

/*
/* …（同文件无关代码省略）… */
    }
    /* Config sanity check */
    if (server.cluster->myself == NULL) goto fmterr;

    zfree(line);
    fclose(fp);
/* …（同文件无关代码省略）… */
void freeClusterLink(clusterLink *link) {
    if (link->conn) {
        connClose(link->conn);
        link->conn = NULL;
    }
    server.stat_cluster_links_memory -= sizeof(list) + listLength(link->send_msg_queue)*sizeof(listNode);
    listRelease(link->send_msg_queue);
    server.stat_cluster_links_memory -= link->rcvbuf_alloc;
    zfree(link->rcvbuf);
    if (link->node) {
        if (link->node->link == link) {
            serverAssert(!link->inbound);
            link->node->link = NULL;
        } else if (link->node->inbound_link == link) {
            serverAssert(link->inbound);
            link->node->inbound_link = NULL;
        }
    }
    zfree(link);
}
/* …（同文件无关代码省略）… */
void setClusterNodeToInboundClusterLink(clusterNode *node, clusterLink *link) {
    serverAssert(!link->node);
    serverAssert(link->inbound);
    if (node->inbound_link) {
        /* A peer may disconnect and then reconnect with us, and it's not guaranteed that
         * we would always process the disconnection of the existing inbound link before
         * accepting a new existing inbound link. Therefore, it's possible to have more than
         * one inbound link from the same node at the same time. Our cleanup logic assumes
         * a one to one relationship between nodes and inbound links, so we need to kill
         * one of the links. The existing link is more likely the outdated one, but it's
         * possible the other node may need to open another link. */
        serverLog(LL_DEBUG, "Replacing inbound link fd %d from node %.40s with fd %d",
                node->inbound_link->conn->fd, node->name, link->conn->fd);
        freeClusterLink(node->inbound_link);
    }
    serverAssert(!node->inbound_link);
    node->inbound_link = link;
    link->node = node;
}
/* …（同文件无关代码省略）… */
clusterNode *clusterLookupNode(const char *name, int length) {
    if (verifyClusterNodeId(name, length) != C_OK) return NULL;
    sds s = sdsnewlen(name, length);
    dictEntry *de = dictFind(server.cluster->nodes, s);
    sdsfree(s);
    if (de == NULL) return NULL;
    return dictGetVal(de);
}
/* …（同文件无关代码省略）… */
    sdsfree(s);
}

/* -----------------------------------------------------------------------------
 * CLUSTER config epoch handling
 * -------------------------------------------------------------------------- */
/* …（同文件无关代码省略）… */
static clusterMsgPingExt *getInitialPingExt(clusterMsg *hdr, int count) {
    clusterMsgPingExt *initial = (clusterMsgPingExt*) &(hdr->data.ping.gossip[count]);
    return initial;
} 
/* …（同文件无关代码省略）… */
uint32_t getAlignedPingExtSize(uint32_t dataSize) {

    return sizeof(clusterMsgPingExt) + EIGHT_BYTE_ALIGN(dataSize);
}
/* …（同文件无关代码省略）… */
uint32_t getHostnamePingExtSize(void) {
    if (sdslen(myself->hostname) == 0) {
        return 0;
    }
    return getAlignedPingExtSize(sdslen(myself->hostname) + 1);
}
/* …（同文件无关代码省略）… */
uint32_t getHumanNodenamePingExtSize(void) {
    if (sdslen(myself->human_nodename) == 0) {
        return 0;
    }
    return getAlignedPingExtSize(sdslen(myself->human_nodename) + 1);
}
/* …（同文件无关代码省略）… */
uint32_t getShardIdPingExtSize(void) {
    return getAlignedPingExtSize(sizeof(clusterMsgPingExtShardId));
}
/* …（同文件无关代码省略）… */
uint32_t getForgottenNodeExtSize(void) {
    return getAlignedPingExtSize(sizeof(clusterMsgPingExtForgottenNode));
}
/* …（同文件无关代码省略）… */
void *preparePingExt(clusterMsgPingExt *ext, uint16_t type, uint32_t length) {
    ext->type = htons(type);
    ext->length = htonl(length);
    return &ext->ext[0];
}
/* …（同文件无关代码省略）… */
clusterMsgPingExt *nextPingExt(clusterMsgPingExt *ext) {
    return (clusterMsgPingExt *)((char*)ext + ntohl(ext->length));
}
/* …（同文件无关代码省略）… */
uint32_t writePingExt(clusterMsg *hdr, int gossipcount)  {
    uint16_t extensions = 0;
    uint32_t totlen = 0;
    clusterMsgPingExt *cursor = NULL;
    /* Set the initial extension position */
    if (hdr != NULL) {
        cursor = getInitialPingExt(hdr, gossipcount);
    }

    /* hostname is optional */
    if (sdslen(myself->hostname) != 0) {
        if (cursor != NULL) {
            /* Populate hostname */
            clusterMsgPingExtHostname *ext = preparePingExt(cursor, CLUSTERMSG_EXT_TYPE_HOSTNAME, getHostnamePingExtSize());
            memcpy(ext->hostname, myself->hostname, sdslen(myself->hostname));

            /* Move the write cursor */
            cursor = nextPingExt(cursor);
        }

        totlen += getHostnamePingExtSize();
        extensions++;
    }

    if (sdslen(myself->human_nodename) != 0) {
        if (cursor != NULL) {
            /* Populate human_nodename */
            clusterMsgPingExtHumanNodename *ext = preparePingExt(cursor, CLUSTERMSG_EXT_TYPE_HUMAN_NODENAME, getHumanNodenamePingExtSize());
            memcpy(ext->human_nodename, myself->human_nodename, sdslen(myself->human_nodename));
        
            /* Move the write cursor */
            cursor = nextPingExt(cursor);
        }

        totlen += getHumanNodenamePingExtSize();
        extensions++;
    }

    /* Gossip forgotten nodes */
    if (dictSize(server.cluster->nodes_black_list) > 0) {
        dictIterator *di = dictGetIterator(server.cluster->nodes_black_list);
        dictEntry *de;
        while ((de = dictNext(di)) != NULL) {
            if (cursor != NULL) {
                uint64_t expire = dictGetUnsignedIntegerVal(de);
                if ((time_t)expire < server.unixtime) continue; /* already expired */
                uint64_t ttl = expire - server.unixtime;
                clusterMsgPingExtForgottenNode *ext = preparePingExt(cursor, CLUSTERMSG_EXT_TYPE_FORGOTTEN_NODE, getForgottenNodeExtSize());
                memcpy(ext->name, dictGetKey(de), CLUSTER_NAMELEN);
                ext->ttl = htonu64(ttl);

                /* Move the write cursor */
                cursor = nextPingExt(cursor);
            }
            totlen += getForgottenNodeExtSize();
            extensions++;
        }
        dictReleaseIterator(di);
    }

    /* Populate shard_id */
    if (cursor != NULL) {
        clusterMsgPingExtShardId *ext = preparePingExt(cursor, CLUSTERMSG_EXT_TYPE_SHARDID, getShardIdPingExtSize());
        memcpy(ext->shard_id, myself->shard_id, CLUSTER_NAMELEN);

        /* Move the write cursor */
        cursor = nextPingExt(cursor);
    }
    totlen += getShardIdPingExtSize();
    extensions++;

    if (hdr != NULL) {
        if (extensions != 0) {
            hdr->mflags[0] |= CLUSTERMSG_FLAG0_EXT_DATA;
        }
        hdr->extensions = htons(extensions);
    }

    return totlen;
}
/* …（同文件无关代码省略）… */
static clusterNode *getNodeFromLinkAndMsg(clusterLink *link, clusterMsg *hdr) {
    clusterNode *sender;
    if (link->node && !nodeInHandshake(link->node)) {
        /* If the link has an associated node, use that so that we don't have to look it
         * up every time, except when the node is still in handshake, the node still has
         * a random name thus not truly "known". */
        sender = link->node;
    } else {
        /* Otherwise, fetch sender based on the message */
        sender = clusterLookupNode(hdr->sender, CLUSTER_NAMELEN);
        /* We know the sender node but haven't associate it with the link. This must
         * be an inbound link because only for inbound links we didn't know which node
         * to associate when they were created. */
        if (sender && !link->node) {
            setClusterNodeToInboundClusterLink(sender, link);
        }
    }
    return sender;
}
/* …（同文件无关代码省略）… */
    }

    sender = getNodeFromLinkAndMsg(link, hdr);

    /* Update the last time we saw any data from this node. We
     * use this in order to avoid detecting a timeout from a node that
/* …（同文件无关代码省略）… */
    /* Set the message flags. */
    if (clusterNodeIsMaster(myself) && server.cluster->mf_end)
        hdr->mflags[0] |= CLUSTERMSG_FLAG0_PAUSED;

    hdr->totlen = htonl(msglen);
}
/* …（同文件无关代码省略）… */
     * to put inside the packet. */
    estlen = sizeof(clusterMsg) - sizeof(union clusterMsgData);
    estlen += (sizeof(clusterMsgDataGossip)*(wanted + pfail_wanted));
    estlen += writePingExt(NULL, 0);
    /* Note: clusterBuildMessageHdr() expects the buffer to be always at least
     * sizeof(clusterMsg) or more. */
    if (estlen < (int)sizeof(clusterMsg)) estlen = sizeof(clusterMsg);
/* …（同文件无关代码省略）… */

    /* Compute the actual total length and send! */
    uint32_t totlen = 0;
    totlen += writePingExt(hdr, gossipcount);
    totlen += sizeof(clusterMsg)-sizeof(union clusterMsgData);
    totlen += (sizeof(clusterMsgDataGossip)*gossipcount);
    serverAssert(gossipcount < USHRT_MAX);
/* …（同文件无关代码省略）… */
/* Add the shard reply of a single shard based off the given primary node. */
void addShardReplyForClusterShards(client *c, list *nodes) {
    serverAssert(listLength(nodes) > 0);
    clusterNode *n = listNodeValue(listFirst(nodes));
    addReplyMapLen(c, 2);
    addReplyBulkCString(c, "slots");

    /* Use slot_info_pairs from the primary only */
    n = clusterNodeGetMaster(n);

    if (n->slot_info_pairs != NULL) {
        serverAssert((n->slot_info_pairs_count % 2) == 0);
        addReplyArrayLen(c, n->slot_info_pairs_count);
        for (int i = 0; i < n->slot_info_pairs_count; i++)
/* …（同文件无关代码省略）… */
int clusterNodeIsMaster(clusterNode *n) {
    return n->flags & CLUSTER_NODE_MASTER;
}
/* …（同文件无关代码省略）… */
clusterNode *clusterNodeGetMaster(clusterNode *node) {
    while (node->slaveof != NULL) node = node->slaveof;
    return node;
}
