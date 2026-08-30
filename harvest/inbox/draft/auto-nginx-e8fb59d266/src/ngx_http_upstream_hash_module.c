// AUTO-DRAFT from nginx/nginx PR #1167
pc->cached = 0;
    pc->connection = NULL;

    for ( ;; ) {

        /*
        }
    }

    hp->rrp.current = peer;
    ngx_http_upstream_rr_peer_ref(hp->rrp.peers, peer);

    pc->sockaddr = peer->sockaddr;
    pc->socklen = peer->socklen;
    pc->name = &peer->name;

    peer->conns++;

    if (now - peer->checked > peer->fail_timeout) {
    points = hcf->points;
    point = &points->point[0];

    for ( ;; ) {
        server = point[hp->hash % points->number].server;

    pc->socklen = best->socklen;
    pc->name = &best->name;

    best->conns++;

    if (now - best->checked > best->fail_timeout) {
