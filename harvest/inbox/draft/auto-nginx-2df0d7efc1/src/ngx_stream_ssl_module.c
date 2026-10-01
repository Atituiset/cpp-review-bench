// AUTO-DRAFT from nginx/nginx PR #938
NULL },
  // <<< BUG ANCHOR
    { ngx_string("ssl_ocsp"),
      NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_FLAG,
      ngx_conf_set_enum_slot,
      NGX_STREAM_SRV_CONF_OFFSET,
      offsetof(ngx_stream_ssl_srv_conf_t, ocsp),
