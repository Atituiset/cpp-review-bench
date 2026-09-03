// AUTO-DRAFT from nginx/nginx PR #1433
ngx_http_secure_link_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char                       *p, *last;  // <<< BUG ANCHOR
    ngx_str_t                     val, hash;
    time_t                        expires;
    ngx_md5_t                     md5;
    ngx_http_secure_link_ctx_t   *ctx;
    ngx_http_secure_link_conf_t  *conf;
    u_char                        hash_buf[18], md5_buf[16];
    ngx_md5_update(&md5, val.data, val.len);
    ngx_md5_final(md5_buf, &md5);

    if (ngx_memcmp(hash_buf, md5_buf, 16) != 0) {
        goto not_found;
    }

    ngx_http_secure_link_conf_t *conf, ngx_http_variable_value_t *v,
    uintptr_t data)
{
    u_char      *p, *start, *end, *last;
    size_t       len;
    ngx_int_t    n;
    ngx_uint_t   i;
    ngx_md5_t    md5;
    u_char       hash[16];

    p = &r->unparsed_uri.data[1];
    ngx_md5_update(&md5, conf->secret.data, conf->secret.len);
    ngx_md5_final(hash, &md5);

    for (i = 0; i < 16; i++) {
        n = ngx_hextoi(&start[2 * i], 2);
        if (n == NGX_ERROR || n != hash[i]) {
            goto not_found;
        }
    }

    v->len = len;
