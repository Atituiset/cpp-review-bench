// AUTO-DRAFT from nginx/nginx PR #258
#define NGX_LISTEN_BACKLOG        -1


#ifdef __DragonFly__
#define NGX_KEEPALIVE_FACTOR      1000
#endif
