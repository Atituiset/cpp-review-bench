// AUTO-DRAFT from redis/redis PR #16074
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct pubsubtype {
    int shard;
    dict *(*clientPubSubChannels)(client*);
    int (*subscriptionCount)(client*);
    kvstore **serverPubSubChannels;
    robj **subscribeMsg;
    robj **unsubscribeMsg;
    robj **messageBulk;
}pubsubtype;
/* …（同文件无关代码省略）… */
void addReplyPubsubMessage(client *c, robj *channel, robj *msg, robj *message_bulk) {
    uint64_t old_flags = c->flags;
    c->flags |= CLIENT_PUSHING;
    if (c->resp == 2)
        addReply(c,shared.mbulkhdr[3]);
    else
        addReplyPushLen(c,3);
    addReply(c,message_bulk);
    addReplyBulk(c,channel);
    if (msg) addReplyBulk(c,msg);
    if (!(old_flags & CLIENT_PUSHING)) c->flags &= ~CLIENT_PUSHING;
}
/* …（同文件无关代码省略）… */
void addReplyPubsubPatMessage(client *c, robj *pat, robj *channel, robj *msg) {
    uint64_t old_flags = c->flags;
    c->flags |= CLIENT_PUSHING;
    if (c->resp == 2)
        addReply(c,shared.mbulkhdr[4]);
    else
        addReplyPushLen(c,4);
    addReply(c,shared.pmessagebulk);
    addReplyBulk(c,pat);
    addReplyBulk(c,channel);
    addReplyBulk(c,msg);
    if (!(old_flags & CLIENT_PUSHING)) c->flags &= ~CLIENT_PUSHING;
}
/* …（同文件无关代码省略）… */
int pubsubPublishMessageInternal(robj *channel, robj *message, pubsubtype type) {
    int receivers = 0;
    dictEntry *de;
    dictIterator di;
    unsigned int slot = 0;

    /* Send to clients listening for that channel */
    if (server.cluster_enabled && type.shard) {
        slot = keyHashSlot(channel->ptr, sdslen(channel->ptr));
    }
    de = kvstoreDictFind(*type.serverPubSubChannels, slot, channel);
    if (de) {
        dict *clients = dictGetVal(de);
        dictEntry *entry;
        dictIterator iter;

        dictInitIterator(&iter, clients);
        while ((entry = dictNext(&iter)) != NULL) {
            client *c = dictGetKey(entry);
            addReplyPubsubMessage(c,channel,message,*type.messageBulk);
            if (clusterSlotStatsEnabled(CLUSTER_SLOT_STATS_NET))
                clusterSlotStatsAddNetworkBytesOutForShardedPubSubInternalPropagation(c, slot);
            updateClientMemUsageAndBucket(c);
            receivers++;
        }
        dictResetIterator(&iter);
    }

    if (type.shard) {
        /* Shard pubsub ignores patterns. */
        return receivers;
    }

    /* Send to clients listening to matching channels */
    if (dictSize(server.pubsub_patterns) > 0) {
        channel = getDecodedObject(channel);
        dictInitIterator(&di, server.pubsub_patterns);
        while((de = dictNext(&di)) != NULL) {
            robj *pattern = dictGetKey(de);
            dict *clients = dictGetVal(de);
            if (!stringmatchlen((char*)pattern->ptr,
                                sdslen(pattern->ptr),
                                (char*)channel->ptr,
                                sdslen(channel->ptr),0)) continue;

            dictEntry *entry;
            dictIterator iter;

            dictInitIterator(&iter, clients);
            while ((entry = dictNext(&iter)) != NULL) {
                client *c = dictGetKey(entry);
                addReplyPubsubPatMessage(c,pattern,channel,message);
                updateClientMemUsageAndBucket(c);
                receivers++;
            }
            dictResetIterator(&iter);
        }
        decrRefCount(channel);
        dictResetIterator(&di);
    }
    return receivers;
}
/* …（同文件无关代码省略）… */
int pubsubPublishMessage(robj *channel, robj *message, int sharded) {
    return pubsubPublishMessageInternal(channel, message, sharded? pubSubShardType : pubSubType);
}
/* …（同文件无关代码省略）… */
/* This function wraps pubsubPublishMessage and also propagates the message to cluster.
 * Used by the commands PUBLISH/SPUBLISH and their respective module APIs.*/
int pubsubPublishMessageAndPropagateToCluster(robj *channel, robj *message, int sharded) {
    int receivers = pubsubPublishMessage(channel, message, sharded);
    if (server.cluster_enabled)
        clusterPropagatePublish(channel, message, sharded);
/* …（同文件无关代码省略）… */
    }

    int receivers = pubsubPublishMessageAndPropagateToCluster(c->argv[1],c->argv[2],0);
    if (!server.cluster_enabled)
        forceCommandPropagation(c,PROPAGATE_REPL);
    addReplyLongLong(c,receivers);
/* …（同文件无关代码省略）… */
/* SPUBLISH <shardchannel> <message> */
void spublishCommand(client *c) {
    int receivers = pubsubPublishMessageAndPropagateToCluster(c->argv[1],c->argv[2],1);
    if (!server.cluster_enabled)
        forceCommandPropagation(c,PROPAGATE_REPL);
    addReplyLongLong(c,receivers);
