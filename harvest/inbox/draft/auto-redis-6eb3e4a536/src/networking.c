// AUTO-DRAFT from redis/redis PR #15510
/* Remove from the list of pending writes if needed. */
    if (c->flags & CLIENT_PENDING_WRITE) {
        serverAssert(&c->clients_pending_write_node.next != NULL ||   // <<< BUG ANCHOR
                     &c->clients_pending_write_node.prev != NULL);
        listUnlinkNode(server.clients_pending_write, &c->clients_pending_write_node);
        c->flags &= ~CLIENT_PENDING_WRITE;
    }
