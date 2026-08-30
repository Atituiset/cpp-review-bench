// AUTO-DRAFT from redis/redis PR #15187
explen = sizeof(clusterMsg)-sizeof(union clusterMsgData);
        explen += sizeof(clusterMsgDataFail);
    } else if (type == CLUSTERMSG_TYPE_PUBLISH || type == CLUSTERMSG_TYPE_PUBLISHSHARD) {
        explen = sizeof(clusterMsg)-sizeof(union clusterMsgData);
        explen += sizeof(clusterMsgDataPublish) -  // <<< BUG ANCHOR
                8 +
                ntohl(hdr->data.publish.msg.channel_len) +
                ntohl(hdr->data.publish.msg.message_len);
    } else if (type == CLUSTERMSG_TYPE_FAILOVER_AUTH_REQUEST ||
               type == CLUSTERMSG_TYPE_FAILOVER_AUTH_ACK ||
               type == CLUSTERMSG_TYPE_MFSTART)
        explen = sizeof(clusterMsg)-sizeof(union clusterMsgData);
        explen += sizeof(clusterMsgDataUpdate);
    } else if (type == CLUSTERMSG_TYPE_MODULE) {
        explen = sizeof(clusterMsg)-sizeof(union clusterMsgData);
        explen += sizeof(clusterMsgModule) -
                3 + ntohl(hdr->data.module.msg.len);
    } else {
        /* We don't know this type of packet, so we assume it's well formed. */
        explen = totlen;
