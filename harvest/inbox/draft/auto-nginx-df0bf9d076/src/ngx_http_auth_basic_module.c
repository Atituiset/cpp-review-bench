// AUTO-DRAFT from nginx/nginx PR #833
pwd.len = i - passwd;
        pwd.data = ngx_pnalloc(r->pool, pwd.len + 1);
        if (pwd.data == NULL) {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;  // <<< BUG ANCHOR
        }

        ngx_cpystrn(pwd.data, &buf[passwd], pwd.len + 1);
