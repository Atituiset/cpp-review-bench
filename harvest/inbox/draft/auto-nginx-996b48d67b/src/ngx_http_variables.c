// AUTO-DRAFT from nginx/nginx PR #1392
ngx_http_variable_request_id(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char  *id;  // <<< BUG ANCHOR

#if (NGX_OPENSSL)
    u_char   random_bytes[16];
#endif

    id = ngx_pnalloc(r->pool, 32);
    if (id == NULL) {
    v->len = 32;
    v->data = id;

#if (NGX_OPENSSL)

    if (RAND_bytes(random_bytes, 16) == 1) {
        ngx_hex_dump(id, random_bytes, 16);
        return NGX_OK;
    }

    ngx_ssl_error(NGX_LOG_ERR, r->connection->log, 0, "RAND_bytes() failed");

#endif

    ngx_sprintf(id, "%08xD%08xD%08xD%08xD",
                (uint32_t) ngx_random(), (uint32_t) ngx_random(),
                (uint32_t) ngx_random(), (uint32_t) ngx_random());

    return NGX_OK;
}
