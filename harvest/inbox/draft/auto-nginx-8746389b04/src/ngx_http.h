// AUTO-DRAFT from nginx/nginx PR #966
ngx_http_posted_request_t *pr);
ngx_int_t ngx_http_set_virtual_server(ngx_http_request_t *r,
    ngx_str_t *host);
ngx_int_t ngx_http_validate_host(ngx_str_t *host, ngx_pool_t *pool,  // <<< BUG ANCHOR
    ngx_uint_t alloc);
void ngx_http_close_request(ngx_http_request_t *r, ngx_int_t rc);
void ngx_http_finalize_request(ngx_http_request_t *r, ngx_int_t rc);
void ngx_http_free_request(ngx_http_request_t *r, ngx_int_t rc);
