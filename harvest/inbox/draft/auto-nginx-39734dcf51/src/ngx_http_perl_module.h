// AUTO-DRAFT from nginx/nginx PR #1556
} ngx_http_perl_var_t;


extern ngx_module_t  ngx_http_perl_module;


/*
 * workaround for "unused variable `Perl___notused'" warning
 * when building with perl 5.6.1

void ngx_http_perl_handle_request(ngx_http_request_t *r);
void ngx_http_perl_sleep_handler(ngx_http_request_t *r);


#endif /* _NGX_HTTP_PERL_MODULE_H_INCLUDED_ */
