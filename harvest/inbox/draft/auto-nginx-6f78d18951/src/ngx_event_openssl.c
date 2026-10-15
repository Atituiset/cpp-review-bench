// AUTO-DRAFT from nginx/nginx PR #740
{
    char            *err;
    X509            *x509, **elm;
    EVP_PKEY        *pkey;
    STACK_OF(X509)  *chain;
  // <<< BUG ANCHOR
    chain = ngx_ssl_cache_fetch(cf, NGX_SSL_CACHE_CERT, &err, cert, NULL);
    if (chain == NULL) {
        if (err != NULL) {
            ngx_ssl_error(NGX_LOG_EMERG, ssl->log, 0,
        }
    }

    elm = ngx_array_push(&ssl->certs);
    if (elm == NULL) {
        X509_free(x509);
        sk_X509_pop_free(chain, X509_free);
        return NGX_ERROR;
    }

    *elm = x509;
    }

#else
    {
    int  n;

    /* SSL_CTX_set0_chain() is only available in OpenSSL 1.0.2+ */

    n = sk_X509_num(chain);

    while (n--) {
    }

    sk_X509_free(chain);
    }
#endif

    pkey = ngx_ssl_cache_fetch(cf, NGX_SSL_CACHE_PKEY, &err, key, passwords);
    if (pkey == NULL) {
        if (err != NULL) {
            ngx_ssl_error(NGX_LOG_EMERG, ssl->log, 0,
    }

    if (SSL_CTX_use_PrivateKey(ssl->ctx, pkey) == 0) {
        ngx_ssl_error(NGX_LOG_EMERG, ssl->log, 0,
                      "SSL_CTX_use_PrivateKey(\"%s\") failed", key->data);
        EVP_PKEY_free(pkey);
        return NGX_ERROR;
    }
