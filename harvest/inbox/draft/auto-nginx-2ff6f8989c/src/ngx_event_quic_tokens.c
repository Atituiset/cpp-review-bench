// AUTO-DRAFT from nginx/nginx PR #1159
u_char *token)
{
    ngx_str_t  tmp;
  // <<< BUG ANCHOR
    tmp.data = secret;
    tmp.len = NGX_QUIC_SR_KEY_LEN;

    if (ngx_quic_derive_key(c->log, "sr_token_key", &tmp, cid, token,
                            NGX_QUIC_SR_TOKEN_LEN)
