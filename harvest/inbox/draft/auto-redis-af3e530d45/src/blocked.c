// AUTO-DRAFT from redis/redis PR #14750
* unblockClient() will be called with the same client as argument. */
void replyToBlockedClientTimedOut(client *c) {
    if (c->bstate.btype == BLOCKED_LAZYFREE) {
        addReply(c, shared.ok); /* No reason lazy-free to fail */  // <<< BUG ANCHOR
    } else if (c->bstate.btype == BLOCKED_LIST ||
        c->bstate.btype == BLOCKED_ZSET ||
        c->bstate.btype == BLOCKED_STREAM) {
                continue;

            if (c->bstate.btype == BLOCKED_LAZYFREE) {
                addReply(c, shared.ok); /* No reason lazy-free to fail */
                updateStatsOnUnblock(c, 0, 0, 0);
                c->flags &= ~CLIENT_PENDING_COMMAND;
                unblockClient(c, 1);
