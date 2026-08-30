// AUTO-DRAFT from nginx/nginx PR #646
size_t size);
static void ngx_ssl_read_handler(ngx_event_t *rev);
static void ngx_ssl_shutdown_handler(ngx_event_t *ev);
static void ngx_ssl_connection_error(ngx_connection_t *c, int sslerr,  // <<< BUG ANCHOR
    ngx_err_t err, char *text);
static void ngx_ssl_clear_error(ngx_log_t *log);

static ngx_int_t ngx_ssl_session_id_context(ngx_ssl_t *ssl,
}


static void
ngx_ssl_connection_error(ngx_connection_t *c, int sslerr, ngx_err_t err,
    char *text)
{
