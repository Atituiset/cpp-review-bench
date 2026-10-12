// AUTO-DRAFT from nginx/nginx PR #1358
if (ngx_handle_write_event(wev, 0) != NGX_OK) {
        ngx_mail_proxy_internal_server_error(s);
    }

    if (c->read->ready) {
