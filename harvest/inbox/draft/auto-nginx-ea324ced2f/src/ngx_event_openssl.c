// AUTO-DRAFT from nginx/nginx PR #634
ngx_int_t
ngx_ssl_dhparam(ngx_conf_t *cf, ngx_ssl_t *ssl, ngx_str_t *file)
{
    BIO  *bio;

    if (file->len == 0) {

    BIO_free(bio);

    return NGX_OK;
}
