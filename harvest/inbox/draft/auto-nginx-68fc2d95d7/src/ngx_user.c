// AUTO-DRAFT from nginx/nginx PR #514
return NGX_ERROR;
}

#else

ngx_int_t
ngx_libc_crypt(ngx_pool_t *pool, u_char *key, u_char *salt, u_char **encrypted)
    return NGX_ERROR;
}

#endif

#endif /* NGX_CRYPT */
