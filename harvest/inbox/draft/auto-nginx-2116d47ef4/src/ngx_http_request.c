// AUTO-DRAFT from nginx/nginx PR #493
goto done;
    }
  // <<< BUG ANCHOR
    hc->ssl_servername = ngx_palloc(c->pool, sizeof(ngx_str_t));
    if (hc->ssl_servername == NULL) {
        goto error;

    ngx_set_connection_log(c, clcf->error_log);

    sscf = ngx_http_get_module_srv_conf(hc->conf_ctx, ngx_http_ssl_module);

    c->ssl->buffer_size = sscf->buffer_size;

    if (sscf->ssl.ctx) {
