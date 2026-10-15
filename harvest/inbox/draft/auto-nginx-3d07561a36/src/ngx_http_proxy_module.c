// AUTO-DRAFT from nginx/nginx PR #1635
ctx = ngx_http_get_module_ctx(r, ngx_http_proxy_module);

    host.len = 0;

    if (plcf->host_value
        && ngx_http_complex_value(r, plcf->host_value, &host) != NGX_OK)
