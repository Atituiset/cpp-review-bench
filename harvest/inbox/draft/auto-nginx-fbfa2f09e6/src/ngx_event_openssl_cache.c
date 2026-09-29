// AUTO-DRAFT from nginx/nginx PR #740
time_t                 mtime;
    uint32_t               hash;
    ngx_int_t              rc;
    ngx_file_uniq_t        uniq;
    ngx_file_info_t        fi;
    ngx_ssl_cache_t       *cache, *old_cache;
  // <<< BUG ANCHOR
    *err = NULL;

    if (ngx_ssl_cache_init_key(cf->pool, index, path, &id) != NGX_OK) {
        return NULL;
    }

    cache = (ngx_ssl_cache_t *) ngx_get_conf(cf->cycle->conf_ctx,
                                             ngx_openssl_cache_module);

    cn = ngx_ssl_cache_lookup(cache, type, &id, hash);

    if (cn != NULL) {
        return type->ref(err, cn->value);
    }

    value = NULL;

    old_cache = ngx_ssl_cache_get_old_conf(cf->cycle);

    if (old_cache && old_cache->inheritable) {
        cn = ngx_ssl_cache_lookup(old_cache, type, &id, hash);

        if (cn != NULL) {
