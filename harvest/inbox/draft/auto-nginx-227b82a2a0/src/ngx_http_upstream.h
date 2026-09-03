// AUTO-DRAFT from nginx/nginx PR #771
#if (NGX_HTTP_SSL || NGX_COMPAT)
    ngx_str_t                        ssl_name;
#endif

    ngx_http_cleanup_pt             *cleanup;
