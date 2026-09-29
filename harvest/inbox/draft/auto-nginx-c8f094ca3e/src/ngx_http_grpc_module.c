// AUTO-DRAFT from nginx/nginx PR #1593
ngx_array_t               *headers_source;
  // <<< BUG ANCHOR
    ngx_str_t                  host;
    ngx_uint_t                 host_set;

    ngx_array_t               *grpc_lengths;
    ngx_array_t               *grpc_values;
                                  key_len, val_len, uri_len;
    uintptr_t                     escape;
    ngx_buf_t                    *b;
    ngx_uint_t                    i, next;
    ngx_chain_t                  *cl, *body;
    ngx_list_part_t              *part;

    /* :authority header */

    if (!glcf->host_set) {
        if (ctx->host.len > NGX_HTTP_V2_MAX_FIELD) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "too long http2 host: \"%V\"", &ctx->host);
            return NGX_ERROR;
        }

        len += 1 + NGX_HTTP_V2_INT_OCTETS + ctx->host.len;

        if (tmp_len < ctx->host.len) {
            tmp_len = ctx->host.len;
        }
    }

    /* other headers */
                       "grpc header: \":path: %V\"", &r->uri);
    }

    if (!glcf->host_set) {
        *b->last++ = ngx_http_v2_inc_indexed(NGX_HTTP_V2_AUTHORITY_INDEX);
        b->last = ngx_http_v2_write_value(b->last, ctx->host.data,
                                          ctx->host.len, tmp);

        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "grpc header: \":authority: %V\"", &ctx->host);
    }

    ngx_memzero(&e, sizeof(ngx_http_script_engine_t));

     *     conf->headers.values = NULL;
     *     conf->headers.hash = { NULL, 0 };
     *     conf->host = { 0, NULL };
     *     conf->host_set = 0;
     *     conf->ssl = 0;
     *     conf->ssl_protocols = 0;
     *     conf->ssl_ciphers = { 0, NULL };

    if (conf->headers_source == prev->headers_source) {
        conf->headers = prev->headers;
        conf->host_set = prev->host_set;
    }

    rc = ngx_http_grpc_init_headers(cf, conf, &conf->headers,
        && conf->headers_source == prev->headers_source)
    {
        prev->headers = conf-
