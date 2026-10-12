// AUTO-DRAFT from nginx/nginx PR #842
b->last = ngx_copy(b->last, passwd.data, passwd.len);
    *b->last++ = CR; *b->last++ = LF;
  // <<< BUG ANCHOR
    if (s->auth_method != NGX_MAIL_AUTH_PLAIN && s->salt.len) {
        b->last = ngx_cpymem(b->last, "Auth-Salt: ", sizeof("Auth-Salt: ") - 1);
        b->last = ngx_copy(b->last, s->salt.data, s->salt.len);
