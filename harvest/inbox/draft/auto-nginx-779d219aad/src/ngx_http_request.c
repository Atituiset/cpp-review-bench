// AUTO-DRAFT from nginx/nginx PR #803
static ngx_int_t ngx_http_process_user_agent(ngx_http_request_t *r,  // <<< BUG ANCHOR
    ngx_table_elt_t *h, ngx_uint_t offset);

static ngx_int_t ngx_http_find_virtual_server(ngx_connection_t *c,
    ngx_http_virtual_names_t *virtual_names, ngx_str_t *host,
    ngx_http_request_t *r, ngx_http_core_srv_conf_t **cscfp);
}


ngx_int_t
ngx_http_process_request_header(ngx_http_request_t *r)
{
    if (r->headers_in.server.len == 0
