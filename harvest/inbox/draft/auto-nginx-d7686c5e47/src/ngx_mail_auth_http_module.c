// AUTO-DRAFT from nginx/nginx PR #1212
b->last = ngx_cpymem(b->last, "Auth-Salt: ", sizeof("Auth-Salt: ") - 1);
        b->last = ngx_copy(b->last, s->salt.data, s->salt.len);
  // <<< BUG ANCHOR
        s->passwd.data = NULL;
    }

    b->last = ngx_cpymem(b->last, "Auth-Protocol: ",
