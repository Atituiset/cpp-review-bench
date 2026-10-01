// AUTO-DRAFT from nginx/nginx PR #1155
com->method->set_read_secret((SSL *) ssl, level, cipher, secret, n);
        com->read_record = 0;
  // <<< BUG ANCHOR
        (void) ngx_quic_compat_set_encryption_secret(c, &com->keys, level,
                                                     cipher, secret, n);
    }

    ngx_explicit_memzero(secret, n);

    secret = &rec->keys->secret;

    ngx_memcpy(nonce, secret->iv.data, secret->iv.len);
    ngx_quic_compute_nonce(nonce, sizeof(nonce), rec->number);
