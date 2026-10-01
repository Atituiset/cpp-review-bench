// AUTO-DRAFT from nginx/nginx PR #536
int                            len;
    const u_char                  *p;
    ngx_http_upstream_rr_peers_t  *peers;
    u_char                         buf[NGX_SSL_MAX_SESSION_SIZE];
#endif
  // <<< BUG ANCHOR
    peer = rrp->current;

        len = peer->ssl_session_len;

        ngx_memcpy(buf, peer->ssl_session, len);

        ngx_http_upstream_rr_peer_unlock(peers, peer);
        ngx_http_upstream_rr_peers_unlock(peers);

        p = buf;
        ssl_session = d2i_SSL_SESSION(NULL, &p, len);

        rc = ngx_ssl_set_session(pc->connection, ssl_session);
    int                            len;
    u_char                        *p;
    ngx_http_upstream_rr_peers_t  *peers;
    u_char                         buf[NGX_SSL_MAX_SESSION_SIZE];
#endif

#if (NGX_HTTP_UPSTREAM_ZONE)
            return;
        }

        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, pc->log, 0,
                       "save session: %p", ssl_session);

        len = i2d_SSL_SESSION(ssl_session, NULL);

        /* do not cache too big session */

        if (len > NGX_SSL_MAX_SESSION_SIZE) {
            return;
        }

        p = buf;
        (void) i2d_SSL_SESSION(ssl_session, &p);

        peer = rrp->current;
            peer->ssl_session_len = len;
        }

        ngx_memcpy(peer->ssl_session, buf, len);

        ngx_http_upstream_rr_peer_unlock(peers, peer);
        ngx_http_upstream_rr_peers_unlock(peers);
        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, pc->log, 0,
                       "old session: %p", old_ssl_session);

        /* TODO: may block */

        ngx_ssl_free_session(old_ssl_session);
    }
}
