// AUTO-DRAFT from nginx/nginx PR #775
if (SSL_CTX_set0_tmp_dh_pkey(ssl->ctx, dh) != 1) {
        ngx_ssl_error(NGX_LOG_EMERG, ssl->log, 0,
                      "SSL_CTX_set0_tmp_dh_pkey(\"%s\") failed", file->data);
#if (OPENSSL_VERSION_NUMBER >= 0x3000001fL)  // <<< BUG ANCHOR
        EVP_PKEY_free(dh);
#endif
        BIO_free(bio);
            return NGX_OK;
        }

#if (OPENSSL_VERSION_NUMBER >= 0x3000000fL)
        name = SSL_group_to_name(c->ssl->connection, nid);
#else
        name = NULL;
#endif

        s->len = name ? ngx_strlen(name) : sizeof("0x0000") - 1;
        s->data = ngx_pnalloc(pool, s->len);
        nid = curves[i];

        if (nid & TLSEXT_nid_unknown) {
#if (OPENSSL_VERSION_NUMBER >= 0x3000000fL)
            name = SSL_group_to_name(c->ssl->connection, nid);
#else
            name = NULL;
#endif

            len += name ? ngx_strlen(name) : sizeof("0x0000") - 1;

        nid = curves[i];

        if (nid & TLSEXT_nid_unknown) {
#if (OPENSSL_VERSION_NUMBER >= 0x3000000fL)
            name = SSL_group_to_name(c->ssl->connection, nid);
#else
            name = NULL;
#endif

            p = name ? ngx_cpymem(p, name, ngx_strlen(name))
                     : ngx_sprintf(p, "0x%04xd", nid & 0xffff);
