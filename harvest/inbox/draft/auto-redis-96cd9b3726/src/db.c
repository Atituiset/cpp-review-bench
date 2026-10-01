// AUTO-DRAFT from redis/redis PR #15004
rewriteClientCommandVector(c, 2, shared.del, key);
        keyModified(c, c->db, key, NULL, 1);
        notifyKeyspaceEvent(NOTIFY_GENERIC, "del", key, c->db->id);
        server.dirty++;
    }
  // <<< BUG ANCHOR

    keyModified(c,c->db,c->argv[1],NULL,1);
    keyModified(c,c->db,c->argv[2],o,1);
    notifyKeyspaceEvent(NOTIFY_GENERIC,"rename_from",
        c->argv[1],c->db->id);
    notifyKeyspaceEvent(NOTIFY_GENERIC,"rename_to",
        c->argv[2],c->db->id);
    if (overwritten) {
        notifyKeyspaceEvent(NOTIFY_OVERWRITTEN, "overwritten", c->argv[2], c->db->id);
        if (desttype != srctype)

    keyModified(c,src,c->argv[1],NULL,1);
    keyModified(c,dst,c->argv[1],kv,1);
    notifyKeyspaceEvent(NOTIFY_GENERIC,
                "move_from",c->argv[1],src->id);
    notifyKeyspaceEvent(NOTIFY_GENERIC,
                "move_to",c->argv[1],dst->id);

    server.dirty++;
    addReply(c,shared.cone);
    /* OK! key copied. Signal modification */
    keyModified(c,dst,c->argv[2],kvCopy,1);
    notifyKeyspaceEvent(NOTIFY_GENERIC,"copy_to",c->argv[2],dst->id);

    /* `delete` implies the destination key was overwritten */
    if (delete) {
