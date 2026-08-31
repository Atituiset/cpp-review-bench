// AUTO-DRAFT from redis/redis PR #14721
if (c == NULL)
        return;
  // <<< BUG ANCHOR
    if (c->funcs && c->funcs->close) {
        c->funcs->close(c);
    }
    if (c->privdata && c->free_privdata)
        c->free_privdata(c->privdata);

    if (c->funcs && c->funcs->free_privctx)
        c->funcs->free_privctx(c->privctx);

    memset(c, 0xff, sizeof(*c));
    hi_free(c);
}
