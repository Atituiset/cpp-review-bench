// AUTO-DRAFT from nginx/nginx PR #1625
ctx->state = state;
    b->pos = pos;
  // <<< BUG ANCHOR
    if (ctx->size > NGX_MAX_OFF_T_VALUE - 5) {
        goto invalid;
    }
