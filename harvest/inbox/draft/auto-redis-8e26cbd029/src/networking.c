// AUTO-DRAFT from redis/redis PR #15371
static void clientSetDefaultAuth(client *c) {
    /* If the default user does not require authentication, the user is
     * directly authenticated. */
    c->user = DefaultUser;  // <<< BUG ANCHOR
    c->authenticated = (c->user->flags & USER_FLAG_NOPASS) &&
                       !(c->user->flags & USER_FLAG_DISABLED);
}
    c->ctime = c->lastinteraction = server.unixtime;
    c->io_lastinteraction = 0;
    c->duration = 0;
    clientSetDefaultAuth(c);
    c->replstate = REPL_STATE_NONE;
    c->repl_start_cmd_stream_on_ack = 0;
    if (username != NULL) {
        user *u = ACLGetUserByName(username, sdslen(username));
        if (u && !(u->flags & USER_FLAG_DISABLED)) {
            c->user = u;
            c->authenticated = 1;
            moduleNotifyUserChanged(c);
            serverLog(LL_VERBOSE, "TLS: Auto-authenticated client as %s",
                      server.hide_user_data_from_log ? "*redacted*" : u->name);
}

void deauthenticateAndCloseClient(client *c) {
    c->user = DefaultUser;
    c->authenticated = 0;
    /* We will write replies to this client later, so we can't
