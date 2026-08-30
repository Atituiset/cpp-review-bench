// AUTO-DRAFT from redis/redis PR #15657
}
  // <<< BUG ANCHOR
int tlsProcessPendingData() {
    listIter li;
    listNode *ln;

    int processed = listLength(pending_list);
    listRewind(pending_list,&li);
    while((ln = listNext(&li))) {
        tls_connection *conn = listNodeValue(ln);
        tlsHandleEvent(conn, AE_READABLE);
    }
    return processed;
