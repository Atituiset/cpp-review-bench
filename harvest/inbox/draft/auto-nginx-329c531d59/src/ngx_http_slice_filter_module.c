// AUTO-DRAFT from nginx/nginx PR #1639
ctx->start = slcf->size * (ngx_http_slice_get_start(r) / slcf->size);

        ctx->range.data = p;
        ctx->range.len = ngx_sprintf(p, "bytes=%O-%O", ctx->start,
                                     ctx->start + (off_t) slcf->size - 1)
