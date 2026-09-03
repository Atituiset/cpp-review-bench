// AUTO-DRAFT from nginx/nginx PR #931
ngx_str_t           addr_text;

    int                 type;

    int                 backlog;
    int                 rcvbuf;
    unsigned            keepalive:2;
    unsigned            quic:1;

    unsigned            deferred_accept:1;
    unsigned            delete_deferred:1;
    unsigned            add_deferred:1;
