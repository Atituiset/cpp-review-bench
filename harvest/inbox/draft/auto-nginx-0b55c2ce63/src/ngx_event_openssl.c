// AUTO-DRAFT from nginx/nginx PR #1471
s->data = ngx_pnalloc(pool, s->len);
        if (s->data == NULL) {
            return NGX_ERROR;
        }
