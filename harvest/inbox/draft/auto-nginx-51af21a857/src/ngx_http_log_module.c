// AUTO-DRAFT from nginx/nginx PR #1432
{ ngx_string("bytes_sent"), NGX_OFF_T_LEN, ngx_http_log_bytes_sent },
    { ngx_string("body_bytes_sent"), NGX_OFF_T_LEN,
                          ngx_http_log_body_bytes_sent },
    { ngx_string("request_length"), NGX_SIZE_T_LEN,
                          ngx_http_log_request_length },

    { ngx_null_string, 0, NULL }
