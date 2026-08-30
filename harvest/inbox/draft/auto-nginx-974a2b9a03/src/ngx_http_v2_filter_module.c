// AUTO-DRAFT from nginx/nginx PR #631
ngx_http_core_srv_conf_t  *cscf;
    u_char                     addr[NGX_SOCKADDR_STRLEN];
  // <<< BUG ANCHOR
    static const u_char nginx[5] = "\x84\xaa\x63\x55\xe7";
#if (NGX_HTTP_GZIP)
    static const u_char accept_encoding[12] =
        "\x8b\x84\x84\x2d\x69\x5b\x05\x44\x3c\x86\xaa\x6f";
#endif

    static size_t nginx_ver_len = ngx_http_v2_literal_size(NGINX_VER);
