// AUTO-DRAFT from nginx/nginx PR #1475
tmp_len = 0;

    } else {
        len += 1 + NGX_HTTP_V2_INT_OCTETS + r->method_name.len;
        tmp_len = r->method_name.len;
    }
        uri_len = r->uri.len + escape + sizeof("?") - 1 + r->args.len;
    }

    len += 1 + NGX_HTTP_V2_INT_OCTETS + uri_len;

    if (tmp_len < uri_len) {
    /* :authority header */

    if (!glcf->host_set) {
        len += 1 + NGX_HTTP_V2_INT_OCTETS + ctx->host.len;

        if (tmp_len < ctx->host.len) {
            continue;
        }

        len += 1 + NGX_HTTP_V2_INT_OCTETS + key_len
                 + NGX_HTTP_V2_INT_OCTETS + val_len;

                continue;
            }

            len += 1 + NGX_HTTP_V2_INT_OCTETS + header[i].key.len
                     + NGX_HTTP_V2_INT_OCTETS + header[i].value.len;
