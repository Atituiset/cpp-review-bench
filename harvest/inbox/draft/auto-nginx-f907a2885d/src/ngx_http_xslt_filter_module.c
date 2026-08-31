// AUTO-DRAFT from nginx/nginx PR #1565
if (ngx_http_xslt_add_chunk(r, ctx, cl->buf) != NGX_OK) {

            if (ctx->ctxt->myDoc) {

#if (NGX_HTTP_XSLT_REUSE_DTD)
