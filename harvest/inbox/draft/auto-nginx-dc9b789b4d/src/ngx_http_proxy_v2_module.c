// AUTO-DRAFT from nginx/nginx PR #1474
tmp_len = 0;

    } else {
        len += 1 + NGX_HTTP_V2_INT_OCTETS + method.len;
        tmp_len = method.len;
    }
        return NGX_ERROR;
    }

    len += 1 + NGX_HTTP_V2_INT_OCTETS + uri_len;

    if (tmp_len < uri_len) {
    host = &ctx->ctx.vars.host_header;

    if (!plcf->host_set) {
        len += 1 + NGX_HTTP_V2_INT_OCTETS + host->len;

        if (tmp_len < host->len) {
            continue;
        }

        len += 1 + NGX_HTTP_V2_INT_OCTETS + key_len
                 + NGX_HTTP_V2_INT_OCTETS + val_len;

                continue;
            }

            len += 1 + NGX_HTTP_V2_INT_OCTETS + header[i].key.len
                     + NGX_HTTP_V2_INT_OCTETS + header[i].value.len;
