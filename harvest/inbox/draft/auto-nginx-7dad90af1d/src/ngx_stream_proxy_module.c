// AUTO-DRAFT from nginx/nginx PR #1109
ngx_str_t                        ssl_ciphers;
    ngx_stream_complex_value_t      *ssl_name;
    ngx_flag_t                       ssl_server_name;

    ngx_flag_t                       ssl_verify;
    ngx_uint_t                       ssl_verify_depth;
#if (NGX_STREAM_SSL)

static ngx_int_t ngx_stream_proxy_send_proxy_protocol(ngx_stream_session_t *s);
static char *ngx_stream_proxy_ssl_certificate_cache(ngx_conf_t *cf,
    ngx_command_t *cmd, void *conf);
static char *ngx_stream_proxy_ssl_password_file(ngx_conf_t *cf,
static void ngx_stream_proxy_ssl_handshake(ngx_connection_t *pc);
static void ngx_stream_proxy_ssl_save_session(ngx_connection_t *c);
static ngx_int_t ngx_stream_proxy_ssl_name(ngx_stream_session_t *s);
static ngx_int_t ngx_stream_proxy_ssl_certificate(ngx_stream_session_t *s);
static ngx_int_t ngx_stream_proxy_merge_ssl(ngx_conf_t *cf,
    ngx_stream_proxy_srv_conf_t *conf, ngx_stream_proxy_srv_conf_t *prev);
      offsetof(ngx_stream_proxy_srv_conf_t, ssl_server_name),
      NULL },

    { ngx_string("proxy_ssl_verify"),
      NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
}


static char *
ngx_stream_proxy_ssl_certificate_cache(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf)
        }
    }

    if (pscf->ssl_certificate
        && pscf->ssl_certificate->value.len
        && (pscf->ssl_certificate->lengths
}


static ngx_int_t
ngx_stream_proxy_ssl_certificate(ngx_stream_session_t *s)
{
    conf->ssl_session_reuse = NGX_CONF_UNSET;
    conf->ssl_name = NGX_CONF_UNSET_PTR;
    conf->ssl_server_name = NGX_CONF_UNSET;
    conf->ssl_verify = NGX_CONF_UNSET;
    conf->ssl_verify_depth = NGX_CONF_UNSET_UINT;
    conf->ssl_certificate = NGX_CONF_UNSET_PTR;

    ngx_conf_merge_value(conf->ssl_server_name, prev->ssl_server_name, 0);

    ngx_conf_merge_value(conf->ssl_verify, prev->ssl_verify, 0);

    ngx_conf_merge_uint_value(conf->ssl_verify_depth,
