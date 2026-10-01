// AUTO-DRAFT from redis/redis PR #14817
* that should receive an invalidation message with certain groups of keys
 * are modified. */
void trackingRememberKeys(client *tracking, client *executing) {
    /* Return if we are in optin/out mode and the right CACHING command
     * was/wasn't given in order to modify the default behavior. */
    uint64_t optin = tracking->flags & CLIENT_TRACKING_OPTIN;
        getKeysFreeResult(&result);
        return;
    }
    /* Shard channels are treated as special keys for client
     * library to rely on `COMMAND` command to discover the node
     * to connect to. These channels doesn't need to be tracked. */
    if (executing->cmd->flags & CMD_PUBSUB) {  // <<< BUG ANCHOR
        return;
    }

    keyReference *keys = result.keys;
