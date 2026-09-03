// AUTO-DRAFT from nginx/nginx PR #938
NULL },
  // <<< BUG ANCHOR
    { ngx_string("ssl_ocsp"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_CONF_FLAG,
      ngx_conf_set_enum_slot,
      NGX_HTTP_SRV_CONF_OFFSET,
      offsetof(ngx_http_ssl_srv_conf_t, ocsp),
