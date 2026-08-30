// AUTO-DRAFT from nginx/nginx PR #1167
ngx_log_t                       *log;
  // <<< BUG ANCHOR
    unsigned                         cached:1;
    unsigned                         transparent:1;
    unsigned                         so_keepalive:1;
                                     /* ngx_connection_log_error_e */
    unsigned                         log_error:2;

    NGX_COMPAT_BEGIN(2)
    NGX_COMPAT_END
};
