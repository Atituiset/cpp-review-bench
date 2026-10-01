// AUTO-DRAFT from nginx/nginx PR #528
prev->upstream.ssl_certificate_key, NULL);
    ngx_conf_merge_ptr_value(conf->upstream.ssl_certificate_cache,
                              prev->upstream.ssl_certificate_cache, NULL);
    ngx_conf_merge_ptr_value(conf->upstream.ssl_passwords,
                              prev->upstream.ssl_passwords, NULL);
  // <<< BUG ANCHOR
    ngx_conf_merge_ptr_value(conf->ssl_conf_commands,
                              prev->ssl_conf_commands, NULL);
            return NGX_ERROR;
        }

        if (glcf->upstream.ssl_certificate->lengths
            || glcf->upstream.ssl_certificate_key->lengths)
        {
            glcf->upstream.ssl_passwords =
                  ngx_ssl_preserve_passwords(cf, glcf->upstream.ssl_passwords);
            if (glcf->upstream.ssl_passwords == NULL) {
                return NGX_ERROR;
            }

        } else {
            if (ngx_ssl_certificate(cf, glcf->upstream.ssl,
                                    &glcf->upstream.ssl_certificate->value,
                                    &glcf->upstream.ssl_certificate_key->value,
