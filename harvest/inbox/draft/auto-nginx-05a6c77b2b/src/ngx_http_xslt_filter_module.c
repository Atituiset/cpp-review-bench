// AUTO-DRAFT from nginx/nginx PR #747
{
    xmlParserCtxtPtr ctxt = data;
  // <<< BUG ANCHOR
    size_t                       n;
    va_list                      args;
    ngx_http_xslt_filter_ctx_t  *ctx;
    u_char                       buf[NGX_MAX_ERROR_STR];
    buf[0] = '\0';

    va_start(args, msg);
    n = (size_t) vsnprintf((char *) buf, NGX_MAX_ERROR_STR, msg, args);
    va_end(args);

    while (--n && (buf[n] == CR || buf[n] == LF)) { /* void */ }

    ngx_log_error(NGX_LOG_ERR, ctx->request->connection->log, 0,
                  "libxml2 error: \"%*s\"", n + 1, buf);
}
