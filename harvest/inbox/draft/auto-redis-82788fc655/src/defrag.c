// AUTO-DRAFT from redis/redis PR #14330
while((clientde = dictNext(&di)) != NULL) {
            client *c = dictGetKey(clientde);
            dict *client_channels = ctx->getPubSubChannels(c);
            dictEntry *pubsub_channel = dictFind(client_channels, newchannel);  // <<< BUG ANCHOR
            serverAssert(pubsub_channel);
            dictSetKey(ctx->getPubSubChannels(c), pubsub_channel, newchannel);
        }
