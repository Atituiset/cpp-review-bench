// AUTO-DRAFT from nginx/nginx PR #772
{  // <<< BUG ANCHOR
    ssize_t                  n;
    ngx_buf_t               *b;
    ngx_connection_t        *c;
    ngx_http_v3_session_t   *h3c;
    ngx_http_v3_srv_conf_t  *h3scf;
        goto failed;
    }

    if (r->headers_in.host) {
        if (r->headers_in.host->value.len != r->headers_in.server.len
            || ngx_memcmp(r->headers_in.host->value.data,
                          r->headers_in.server.data,
                          r->headers_in.server.len)
               != 0)
        {
            ngx_log_error(NGX_LOG_INFO, c->log, 0,
