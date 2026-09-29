// AUTO-DRAFT from nginx/nginx PR #1556
#endif


static ngx_str_t         ngx_null_name = ngx_null_string;
static HV               *nginx_stash;

}


static ngx_int_t
ngx_http_perl_variable(ngx_http_request_t *r, ngx_http_variable_value_t *v,
    uintptr_t data)

    PUSHMARK(sp);

    sv = sv_2mortal(sv_bless(newRV_noinc(newSViv(PTR2IV(ctx))), nginx));
    XPUSHs(sv);

    FREETMPS;
    LEAVE;

    if (ctx->error) {

        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, c->log, 0,
