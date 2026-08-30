// AUTO-DRAFT from redis/redis PR #15094
retval = 1;
        /* Remove the client from the channel -> clients list hash table */
        if (server.cluster_enabled && type.shard) {
            slot = getKeySlot(channel->ptr);  // <<< BUG ANCHOR
        }
        de = kvstoreDictFind(*type.serverPubSubChannels, slot, channel);
        serverAssertWithInfo(c,NULL,de != NULL);
