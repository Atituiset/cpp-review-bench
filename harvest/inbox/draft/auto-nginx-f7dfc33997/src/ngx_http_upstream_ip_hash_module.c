// AUTO-DRAFT from nginx/nginx PR #1167
hash = iphp->hash;

    for ( ;; ) {

        for (i = 0; i < (ngx_uint_t) iphp->addrlen; i++) {
        }
    }

    iphp->rrp.current = peer;
    ngx_http_upstream_rr_peer_ref(iphp->rrp.peers, peer);

    pc->sockaddr = peer->sockaddr;
    pc->socklen = peer->socklen;
    pc->name = &peer->name;

    peer->conns++;

    if (now - peer->checked > peer->fail_timeout) {
