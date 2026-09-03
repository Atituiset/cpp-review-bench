// AUTO-DRAFT from redis/redis PR #15468
static void clientSetDefaultAuth(client *c) {
    /* If the default user does not require authentication, the user is
     * directly authenticated. */
    clientSetUser(c, DefaultUser);  // <<< BUG ANCHOR
    c->authenticated = (c->user->flags & USER_FLAG_NOPASS) &&
                       !(c->user->flags & USER_FLAG_DISABLED);
}
        user *u = ACLGetUserByName(username, sdslen(username));
        if (u && !(u->flags & USER_FLAG_DISABLED)) {
            c->authenticated = 1;
            clientSetUser(c, u);
            moduleNotifyUserChanged(c);
            serverLog(LL_VERBOSE, "TLS: Auto-authenticated client as %s",
                      server.hide_user_data_from_log ? "*redacted*" : u->name);
    }
}

/* Clear the client state to resemble a newly connected client. */
void clearClientConnectionState(client *c) {
    listNode *ln;
    c->resp = 2;
#endif

    clientSetDefaultAuth(c);
    moduleNotifyUserChanged(c);
    discardTransaction(c);
    himportFieldsetsFree(c);

    pubsubUnsubscribeAllChannels(c,0);
    pubsubUnsubscribeShardAllChannels(c, 0);
    pubsubUnsubscribeAllPatterns(c,0);
    unmarkClientAsPubSub(c);

    if (c->name) {
        decrRefCount(c->name);
        c->name = NULL;
}

void deauthenticateAndCloseClient(client *c) {
    disableTracking(c);
    c->user = DefaultUser;
    c->authenticated = 0;
    /* We will write replies to this client later, so we can't
    listRelease(c->watched_keys);

    /* Unsubscribe from all the pubsub channels */
    pubsubUnsubscribeAllChannels(c,0);
    pubsubUnsubscribeShardAllChannels(c, 0);
    pubsubUnsubscribeAllPatterns(c,0);
    unmarkClientAsPubSub(c);
    dictRelease(c->pubsub_channels);
    dictRelease(c->pubsub_patterns);
    dictRelease(c->pubsubshard_channels);
