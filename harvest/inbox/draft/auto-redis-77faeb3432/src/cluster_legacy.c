// AUTO-DRAFT from redis/redis PR #16104
/* 标准头由采集器按切片用到的 libc 符号推断补齐 */
#include <stdint.h>
  // <<< BUG ANCHOR
static uint32_t getPingExtLength(clusterMsgPingExt *ext) {
    return ntohl(ext->length);
}
/* …（同文件无关代码省略）… */
static clusterMsgPingExt *getInitialPingExt(clusterMsg *hdr, int count) {
    clusterMsgPingExt *initial = (clusterMsgPingExt*) &(hdr->data.ping.gossip[count]);
    return initial;
} 
/* …（同文件无关代码省略）… */
        explen = sizeof(clusterMsg)-sizeof(union clusterMsgData);
        explen += (sizeof(clusterMsgDataGossip)*count);

        /* If there is extension data, which doesn't have a fixed length,
         * loop through them and validate the length of it now. */
        if (hdr->mflags[0] & CLUSTERMSG_FLAG0_EXT_DATA) {
            clusterMsgPingExt *ext = getInitialPingExt(hdr, count);
            while (extensions--) {
                uint16_t extlen = getPingExtLength(ext);
                if (extlen < sizeof(clusterMsgPingExt) || extlen % 8 != 0) {
                    serverLog(LL_WARNING, "Received a %s packet without proper padding (%d bytes)",
                        clusterGetMessageTypeString(type), (int) extlen);
/* …（同文件无关代码省略）… */
const char *clusterGetMessageTypeString(int type) {
    switch(type) {
    case CLUSTERMSG_TYPE_PING: return "ping";
    case CLUSTERMSG_TYPE_PONG: return "pong";
    case CLUSTERMSG_TYPE_MEET: return "meet";
    case CLUSTERMSG_TYPE_FAIL: return "fail";
    case CLUSTERMSG_TYPE_PUBLISH: return "publish";
    case CLUSTERMSG_TYPE_PUBLISHSHARD: return "publishshard";
    case CLUSTERMSG_TYPE_FAILOVER_AUTH_REQUEST: return "auth-req";
    case CLUSTERMSG_TYPE_FAILOVER_AUTH_ACK: return "auth-ack";
    case CLUSTERMSG_TYPE_UPDATE: return "update";
    case CLUSTERMSG_TYPE_MFSTART: return "mfstart";
    case CLUSTERMSG_TYPE_MODULE: return "module";
    }
    return "unknown";
}
