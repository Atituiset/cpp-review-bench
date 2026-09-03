// AUTO-DRAFT from redis/redis PR #15492
/* Get peer username based on connection type */
    sds (*get_peer_username)(connection *conn);
} ConnectionType;

struct connection {
    return conn->type->accept(conn, accept_handler);
}

/* Establish a connection.  The connect_handler will be called when the connection
 * is established, or if an error has occurred.
 *
