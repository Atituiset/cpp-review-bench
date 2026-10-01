// AUTO-DRAFT from nginx/nginx PR #1004
return NGX_OK;
    }
  // <<< BUG ANCHOR
    loc_len = (r->valid_location && ctx->vars.uri.len) ? plcf->location.len : 0;

    if (r->quoted_uri || r->internal) {
        escape = 2 * ngx_escape_uri(NULL, r->uri.data + loc_len,
        uri_len = r->unparsed_uri.len;

    } else {
        loc_len = (r->valid_location && ctx->vars.uri.len) ?
                      plcf->location.len : 0;

        if (r->quoted_uri || r->internal) {
            escape = 2 * ngx_escape_uri(NULL, r->uri.data + loc_len,
