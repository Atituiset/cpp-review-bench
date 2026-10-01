// AUTO-DRAFT from nginx/nginx PR #966
goto done;
    }
  // <<< BUG ANCHOR
    rc = ngx_http_validate_host(&host, c->pool, 1);

    if (rc == NGX_ERROR) {
        goto error;
    ssize_t              n;
    ngx_int_t            rc, rv;
    ngx_str_t            host;
    ngx_connection_t    *c;
    ngx_http_request_t  *r;

                host.len = r->host_end - r->host_start;
                host.data = r->host_start;

                rc = ngx_http_validate_host(&host, r->pool, 0);

                if (rc == NGX_DECLINED) {
                    ngx_log_error(NGX_LOG_INFO, c->log, 0,
                }

                r->headers_in.server = host;
            }

            if (r->http_version < NGX_HTTP_VERSION_10) {
ngx_http_process_host(ngx_http_request_t *r, ngx_table_elt_t *h,
    ngx_uint_t offset)
{
    u_char     *p;
    ngx_int_t   rc;
    ngx_str_t   host;

    if (r->headers_in.host) {
        ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,

    host = h->value;

    rc = ngx_http_validate_host(&host, r->pool, 0);

    if (rc == NGX_DECLINED) {
        ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
    }

    r->headers_in.server = host;

    p = ngx_strlchr(h->value.data + host.len,
                    h->value.data + h->value.len, ':');

    if (p) {
        rc = ngx_atoi(p + 1, h->value.data + h->value.len - p - 1);

        if (rc > 0 && rc < 65536) {
            r->port = rc;
        }
    }

    return NGX_OK;
}


ngx_int_t
ngx_http_validate_host(ngx_str_t *host, ngx_pool_t *pool, ngx_uint_t alloc)
{
    u_char  *h, ch;
    size_t   i, dot_pos, host_len;

    enum {
        sw_usual = 0,
        sw_literal,
        sw_rest
    } state;

    dot_pos = host->len;
    host_len = host->len;

    h = host->data;

    state = sw_usual;

    for (i = 0; i < host->len; i++) {
        ch = h[i];

        switch (ch) {

        case '.':
            if (dot_pos == i - 1) {
                return NGX_DECLINED;
            }
            dot_pos = i;
            break;

        case ':':
            if (state == 
