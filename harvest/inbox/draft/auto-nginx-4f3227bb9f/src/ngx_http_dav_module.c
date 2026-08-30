// AUTO-DRAFT from nginx/nginx PR #1307
ngx_http_dav_loc_conf_t *dlcf);

static ngx_int_t ngx_http_dav_copy_move_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_dav_copy_dir(ngx_tree_ctx_t *ctx, ngx_str_t *path);
static ngx_int_t ngx_http_dav_copy_dir_time(ngx_tree_ctx_t *ctx,
    ngx_str_t *path);

    r->uri = uri;

    copy.path.len--;  /* omit "\0" */

    if (copy.path.data[copy.path.len - 1] == '/') {
    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http copy to: \"%s\"", copy.path.data);

    if (ngx_link_info(copy.path.data, &fi) == NGX_FILE_ERROR) {
        err = ngx_errno;

}


static ngx_int_t
ngx_http_dav_copy_dir(ngx_tree_ctx_t *ctx, ngx_str_t *path)
{
