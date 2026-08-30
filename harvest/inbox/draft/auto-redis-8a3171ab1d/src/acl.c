// AUTO-DRAFT from redis/redis PR #15356
listRewind(server.clients,&li);
        while ((ln = listNext(&li)) != NULL) {
            client *c = listNodeValue(ln);
            /* a MASTER client can do everything (and user = NULL) so we can skip it */
            if (c->flags & CLIENT_MASTER)  // <<< BUG ANCHOR
                continue;
            user *original = c->user;
            list *channels = NULL;
