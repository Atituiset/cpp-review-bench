// AUTO-DRAFT from redis/redis PR #15329
sendPendingClientsToMainThreadIfNeeded(t, 1);
        /* Disable read and write to avoid race when main thread processes. */
        c->io_flags &= ~(CLIENT_IO_READ_ENABLED | CLIENT_IO_WRITE_ENABLED);
        /* Remove the client from IO thread, add it to main thread's pending list. */
        listUnlinkNode(t->clients, c->io_thread_client_list_node);
        listLinkNodeTail(t->pending_clients_to_main_thread, c->io_thread_client_list_node);
