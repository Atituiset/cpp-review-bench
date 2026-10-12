// AUTO-DRAFT from nginx/nginx PR #1395
void
ngx_http_script_regex_start_code(ngx_http_script_engine_t *e)
{  // <<< BUG ANCHOR
    size_t                         len;
    ngx_int_t                      rc;
    ngx_uint_t                     n;
    if (code->lengths == NULL) {
        e->buf.len = code->size;

        if (code->uri) {
            if (r->ncaptures && (r->quoted_uri || r->plus_in_uri)) {
                e->buf.len += 2 * ngx_escape_uri(NULL, r->uri.data, r->uri.len,
                                                 NGX_ESCAPE_ARGS);
            }
        }

        for (n = 2; n < r->ncaptures; n += 2) {
            e->buf.len += r->captures[n + 1] - r->captures[n];
        }

    } else {
