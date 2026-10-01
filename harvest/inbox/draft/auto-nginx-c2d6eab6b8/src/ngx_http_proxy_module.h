// AUTO-DRAFT from nginx/nginx PR #1593
ngx_http_proxy_headers_t       headers_cache;
#endif
    ngx_array_t                   *headers_source;
    ngx_uint_t                     host_set;  // <<< BUG ANCHOR

    ngx_array_t                   *proxy_lengths;
    ngx_array_t                   *proxy_values;
    unsigned                       head:1;
    unsigned                       internal_chunked:1;
    unsigned                       header_sent:1;
    unsigned                       legacy:1;
} ngx_http_proxy_ctx_t;
