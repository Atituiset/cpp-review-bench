// AUTO-DRAFT from nginx/nginx PR #1635
/* :authority header */

    host.len = 0;

    if (plcf->host_value
        && ngx_http_complex_value(r, plcf->host_value, &host) != NGX_OK)
