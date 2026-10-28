// AUTO-DRAFT from redis/redis PR #16074
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
  // <<< BUG ANCHOR
static inline int defaultClientPort(void) {
    return server.tls_cluster ? server.tls_port : server.port;
}
/* …（同文件无关代码省略）… */
typedef struct {
    size_t totlen; /* Total length of this block including the message */
    int refcount;  /* Number of cluster link send msg queues containing the message */
    clusterMsg msg[];
} clusterMsgSendBlock;
/* …（同文件无关代码省略）… */
static clusterMsg *getMessageFromSendBlock(clusterMsgSendBlock *msgblock) {
    return &msgblock->msg[0];
}
/* …（同文件无关代码省略）… */
void deriveAnnouncedPorts(int *announced_tcp_port, int *announced_tls_port,
                          int *announced_cport) {
    /* Config overriding announced ports. */
    *announced_tcp_port = server.cluster_announce_port ? 
                          server.cluster_announce_port : server.port;
    *announced_tls_port = server.cluster_announce_tls_port ? 
                          server.cluster_announce_tls_port : server.tls_port;
    /* Derive cluster bus port. */
    if (server.cluster_announce_bus_port) {
        *announced_cport = server.cluster_announce_bus_port;
    } else if (server.cluster_port) {
        *announced_cport = server.cluster_port;
    } else {
        *announced_cport = defaultClientPort() + CLUSTER_PORT_INCR;
    }
}
/* …（同文件无关代码省略）… */
static clusterMsgSendBlock *createClusterMsgSendBlock(int type, uint32_t msglen) {
    uint32_t blocklen = msglen + sizeof(clusterMsgSendBlock);
    clusterMsgSendBlock *msgblock = zcalloc(blocklen);
    msgblock->refcount = 1;
    msgblock->totlen = blocklen;
    server.stat_cluster_links_memory += blocklen;
    clusterBuildMessageHdr(getMessageFromSendBlock(msgblock),type,msglen);
    return msgblock;
}
/* …（同文件无关代码省略）… */
static void clusterMsgSendBlockDecrRefCount(void *node) {
    clusterMsgSendBlock *msgblock = (clusterMsgSendBlock*)node;
    msgblock->refcount--;
    serverAssert(msgblock->refcount >= 0);
    if (msgblock->refcount == 0) {
        server.stat_cluster_links_memory -= msgblock->totlen;
        zfree(msgblock);
    }
}
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
void handleLinkIOError(clusterLink *link) {
    freeClusterLink(link);
}
/* …（同文件无关代码省略）… */
void clusterWriteHandler(connection *conn) {
    clusterLink *link = connGetPrivateData(conn);
    ssize_t nwritten;
    size_t totwritten = 0;

    while (totwritten < NET_MAX_WRITES_PER_EVENT && listLength(link->send_msg_queue) > 0) {
        listNode *head = listFirst(link->send_msg_queue);
        clusterMsgSendBlock *msgblock = (clusterMsgSendBlock*)head->value;
        clusterMsg *msg = getMessageFromSendBlock(msgblock);
        size_t msg_offset = link->head_msg_send_offset;
        size_t msg_len = ntohl(msg->totlen);

        nwritten = connWrite(conn, (char*)msg + msg_offset, msg_len - msg_offset);
        if (nwritten <= 0) {
            serverLog(LL_DEBUG,"I/O error writing to node link: %s",
                (nwritten == -1) ? connGetLastError(conn) : "short write");
            handleLinkIOError(link);
            return;
        }
        if (msg_offset + nwritten < msg_len) {
            /* If full message wasn't written, record the offset
             * and continue sending from this point next time */
            link->head_msg_send_offset += nwritten;
            return;
        }
        serverAssert((msg_offset + nwritten) == msg_len);
        link->head_msg_send_offset = 0;

        /* Delete the node and update our memory tracking */
        uint32_t blocklen = msgblock->totlen;
        listDelNode(link->send_msg_queue, head);
        server.stat_cluster_links_memory -= sizeof(listNode);
        link->send_msg_queue_mem -= sizeof(listNode) + blocklen;

        totwritten += nwritten;
    }

    if (listLength(link->send_msg_queue) == 0)
        connSetWriteHandler(link->conn, NULL);
}
/* …（同文件无关代码省略）… */
void clusterSendMessage(clusterLink *link, clusterMsgSendBlock *msgblock) {
    if (!link) {
        return;
    }
    if (listLength(link->send_msg_queue) == 0 && getMessageFromSendBlock(msgblock)->totlen != 0)
        connSetWriteHandlerWithBarrier(link->conn, clusterWriteHandler, 1);

    listAddNodeTail(link->send_msg_queue, msgblock);
    msgblock->refcount++;

    /* Update memory tracking */
    link->send_msg_queue_mem += sizeof(listNode) + msgblock->totlen;
    server.stat_cluster_links_memory += sizeof(listNode);

    /* Populate sent messages stats. */
    uint16_t type = ntohs(getMessageFromSendBlock(msgblock)->type);
    if (type < CLUSTERMSG_TYPE_COUNT)
        server.cluster->stats_bus_messages_sent[type]++;
}
/* …（同文件无关代码省略）… */
void clusterBroadcastMessage(clusterMsgSendBlock *msgblock) {
    dictIterator di;
    dictEntry *de;

    dictInitSafeIterator(&di, server.cluster->nodes);
    while((de = dictNext(&di)) != NULL) {
        clusterNode *node = dictGetVal(de);

        if (node->flags & (CLUSTER_NODE_MYSELF|CLUSTER_NODE_HANDSHAKE))
            continue;
        clusterSendMessage(node->link,msgblock);
    }
    dictResetIterator(&di);
}
/* …（同文件无关代码省略）… */
static void clusterBuildMessageHdr(clusterMsg *hdr, int type, size_t msglen) {
    uint64_t offset;
    clusterNode *master;

    /* If this node is a master, we send its slots bitmap and configEpoch.
     * If this node is a slave we send the master's information instead (the
     * node is flagged as slave so the receiver knows that it is NOT really
     * in charge for this slots. */
    master = (nodeIsSlave(myself) && myself->slaveof) ?
              myself->slaveof : myself;

    hdr->ver = htons(CLUSTER_PROTO_VER);
    hdr->sig[0] = 'R';
    hdr->sig[1] = 'C';
    hdr->sig[2] = 'm';
    hdr->sig[3] = 'b';
    hdr->type = htons(type);
    memcpy(hdr->sender,myself->name,CLUSTER_NAMELEN);

    /* If cluster-announce-ip option is enabled, force the receivers of our
     * packets to use the specified address for this node. Otherwise if the
     * first byte is zero, they'll do auto discovery. */
    memset(hdr->myip,0,NET_IP_STR_LEN);
    if (server.cluster_announce_ip) {
        redis_strlcpy(hdr->myip,server.cluster_announce_ip,NET_IP_STR_LEN);
    }

    /* Handle cluster-announce-[tls-|bus-]port. */
    int announced_tcp_port, announced_tls_port, announced_cport;
    deriveAnnouncedPorts(&announced_tcp_port, &announced_tls_port, &announced_cport);

    memcpy(hdr->myslots,master->slots,sizeof(hdr->myslots));
    memset(hdr->slaveof,0,CLUSTER_NAMELEN);
    if (myself->slaveof != NULL)
        memcpy(hdr->slaveof,myself->slaveof->name, CLUSTER_NAMELEN);
    if (server.tls_cluster) {
        hdr->port = htons(announced_tls_port);
        hdr->pport = htons(announced_tcp_port);
    } else {
        hdr->port = htons(announced_tcp_port);
        hdr->pport = htons(announced_tls_port);
    }
    hdr->cport = htons(announced_cport);
    hdr->flags = htons(myself->flags);
    hdr->state = server.cluster->state;

    /* Set the currentEpoch and configEpochs. */
    hdr->currentEpoch = htonu64(server.cluster->currentEpoch);
    hdr->configEpoch = htonu64(master->configEpoch);

    /* Set the replication offset. */
    if (nodeIsSlave(myself))
        offset = replicationGetSlaveOffset();
    else
        offset = server.master_repl_offset;
    hdr->offset = htonu64(offset);

    /* Set the message flags. */
    if (clusterNodeIsMaster(myself) && server.cluster->mf_end)
        hdr->mflags[0] |= CLUSTERMSG_FLAG0_PAUSED;
    hdr->mflags[0] |= CLUSTERMSG_FLAG0_EXT_DATA; /* Always make other nodes know that
                                                  * this node supports extension data. */

    hdr->totlen = htonl(msglen);
}
/* …（同文件无关代码省略）… */
    dictResetIterator(&di);
}

/* Create a PUBLISH message block.
 *
 * Sanitizer suppression: In clusterMsgDataPublish, sizeof(bulk_data) is 8.
/* …（同文件无关代码省略）… */
clusterMsgSendBlock *clusterCreatePublishMsgBlock(robj *channel, robj *message, uint16_t type) {

    uint32_t channel_len, message_len;

    channel = getDecodedObject(channel);
    message = getDecodedObject(message);
    channel_len = sdslen(channel->ptr);
    message_len = sdslen(message->ptr);

    size_t msglen = sizeof(clusterMsg)-sizeof(union clusterMsgData);
    msglen += sizeof(clusterMsgDataPublish) - 8 + channel_len + message_len;
    clusterMsgSendBlock *msgblock = createClusterMsgSendBlock(type, msglen);

    clusterMsg *hdr = getMessageFromSendBlock(msgblock);
/* …（同文件无关代码省略）… */

    if (!sharded) {
        msgblock = clusterCreatePublishMsgBlock(channel, message, CLUSTERMSG_TYPE_PUBLISH);
        clusterBroadcastMessage(msgblock);
        clusterMsgSendBlockDecrRefCount(msgblock);
        return;
/* …（同文件无关代码省略）… */
    serverAssert(nodes_for_slot != NULL);
    listRewind(nodes_for_slot, &li);
    msgblock = clusterCreatePublishMsgBlock(channel, message, CLUSTERMSG_TYPE_PUBLISHSHARD);
    while((ln = listNext(&li))) {
        clusterNode *node = listNodeValue(ln);
        if (node->flags & (CLUSTER_NODE_MYSELF|CLUSTER_NODE_HANDSHAKE))
/* …（同文件无关代码省略）… */
int clusterNodeIsMaster(clusterNode *n) {
    return n->flags & CLUSTER_NODE_MASTER;
}
