// AUTO-DRAFT from nginx/nginx PR #1359
}

    if (r->headers_out.content_type.len) {
        len += 1 + NGX_HTTP_V2_INT_OCTETS + r->headers_out.content_type.len;

        if (r->headers_out.content_type_len == r->headers_out.content_type.len

    if (r->headers_out.location && r->headers_out.location->value.len) {

        if (r->headers_out.location->value.data[0] == '/'
            && clcf->absolute_redirect)
        {
