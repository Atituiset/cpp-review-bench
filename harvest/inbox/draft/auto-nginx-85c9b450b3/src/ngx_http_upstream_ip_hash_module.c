// AUTO-DRAFT from nginx/nginx PR #208
ngx_http_upstream_rr_peers_rlock(iphp->rrp.peers);
  // <<< BUG ANCHOR
    if (iphp->tries > 20 || iphp->rrp.peers->single) {
        ngx_http_upstream_rr_peers_unlock(iphp->rrp.peers);
        return iphp->get_rr_peer(pc, &iphp->rrp);
    }

    now = ngx_time();

    pc->cached = 0;
    }

    iphp->rrp.current = peer;

    pc->sockaddr = peer->sockaddr;
    pc->socklen = peer->socklen;
    uscf->peer.init_upstream = ngx_http_upstream_init_ip_hash;

    uscf->flags = NGX_HTTP_UPSTREAM_CREATE
                  |NGX_HTTP_UPSTREAM_WEIGHT
                  |NGX_HTTP_UPSTREAM_MAX_CONNS
                  |NGX_HTTP_UPSTREAM_MAX_FAILS
