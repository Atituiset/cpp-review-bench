// AUTO-DRAFT from nginx/nginx PR #1022
ngx_log_debug1(NGX_LOG_DEBUG_EVENT, c->log, 0,
                   "quic ngx_quic_cbs_release_rcd len:%uz", bytes_read);

    qc = ngx_quic_get_connection(c);
    ctx = ngx_quic_get_send_ctx(qc, qc->read_level);

    cl = ngx_quic_read_buffer(c, &ctx->crypto, bytes_read);
