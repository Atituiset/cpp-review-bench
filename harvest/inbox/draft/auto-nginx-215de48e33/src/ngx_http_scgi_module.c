// AUTO-DRAFT from nginx/nginx PR #344
return NGX_CONF_OK;
    }

#if (NGX_HTTP_CACHE)
    if (scf->upstream.cache > 0) {
        return "is incompatible with \"scgi_cache\"";
