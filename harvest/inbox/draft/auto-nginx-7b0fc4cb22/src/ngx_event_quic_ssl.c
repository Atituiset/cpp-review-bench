// AUTO-DRAFT from nginx/nginx PR #889
ngx_log_debug1(NGX_LOG_DEBUG_EVENT, c->log, 0, "SSL_do_handshake: %d", n);
  // <<< BUG ANCHOR
    if (qc->error) {
        return NGX_ERROR;
    }

    if (n <= 0) {
        sslerr = SSL_get_error(ssl_conn, n);

        ngx_log_debug1(NGX_LOG_DEBUG_EVENT, c->log, 0, "SSL_get_error: %d",
                       sslerr);

        if (sslerr != SSL_ERROR_WANT_READ) {

            if (c->ssl->handshake_rejected) {
                ngx_connection_error(c, 0, "handshake rejected");
                ERR_clear_error();

                return NGX_ERROR;
            }

            ngx_ssl_connection_error(c, sslerr, 0, "SSL_do_handshake() failed");
            return NGX_ERROR;
        }
    }

    if (!SSL_is_init_finished(ssl_conn)) {
        if (ngx_quic_keys_available(qc->keys, NGX_QUIC_ENCRYPTION_EARLY_DATA, 0)
            && qc->client_tp_done)
