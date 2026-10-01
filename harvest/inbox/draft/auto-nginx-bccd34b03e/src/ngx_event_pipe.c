// AUTO-DRAFT from nginx/nginx PR #1664
chain->next = NULL;
  // <<< BUG ANCHOR
            } else if (!p->cacheable
                       && p->downstream->data == p->output_ctx
                       && p->downstream->write->ready
                       && !p->downstream->write->delayed)
    ngx_chain_t  *cl, *tl;

    for ( ;; ) {
        if (p->busy) {
            cl = p->busy;
            p->busy = NULL;

        } else if (p->out) {
            cl = p->out;
            p->out = NULL;
