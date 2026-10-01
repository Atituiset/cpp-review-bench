// AUTO-DRAFT from nginx/nginx PR #208
typedef struct {
    ngx_http_complex_value_t            key;
    ngx_http_upstream_chash_points_t   *points;
} ngx_http_upstream_hash_srv_conf_t;
  // <<< BUG ANCHOR

static ngx_int_t ngx_http_upstream_init_chash(ngx_conf_t *cf,
    ngx_http_upstream_srv_conf_t *us);
static int ngx_libc_cdecl
    ngx_http_upstream_chash_cmp_points(const void *one, const void *two);
static ngx_uint_t ngx_http_upstream_find_chash_point(

    ngx_http_upstream_rr_peers_rlock(hp->rrp.peers);

    if (hp->tries > 20 || hp->rrp.peers->single || hp->key.len == 0) {
        ngx_http_upstream_rr_peers_unlock(hp->rrp.peers);
        return hp->get_rr_peer(pc, &hp->rrp);
    }

    now = ngx_time();

    }

    hp->rrp.current = peer;

    pc->sockaddr = peer->sockaddr;
    pc->socklen = peer->socklen;

static ngx_int_t
ngx_http_upstream_init_chash(ngx_conf_t *cf, ngx_http_upstream_srv_conf_t *us)
{
    u_char                             *host, *port, c;
    size_t                              host_len, port_len, size;
        u_char                          byte[4];
    } prev_hash;

    if (ngx_http_upstream_init_round_robin(cf, us) != NGX_OK) {
        return NGX_ERROR;
    }

    us->peer.init = ngx_http_upstream_init_chash_peer;

    peers = us->peer.data;
    npoints = peers->total_weight * 160;

    size = sizeof(ngx_http_upstream_chash_points_t)
           + sizeof(ngx_http_upstream_chash_point_t) * (npoints - 1);

    points = ngx_palloc(cf->pool, size);
    if (points == NULL) {
        return NGX_ERROR;
    }

    points->number = 0;

    for (peer = peers->peer; peer; peer = peer->next) {
        server = &peer->server;


    points->number = i + 1;

    hcf = ngx_http_conf_upstream_srv_conf(us, ngx_http_upstream_hash_module);
    hcf->points = points;

    return NGX_OK;

    ngx_http_upstream_rr_peers_rlock(hp->rrp.peers);

    hp->hash = ngx_http_upstream_find_chash_point(hcf->points, hash);

    ngx_http_upstream_rr_peers_unlock(hp->rrp.peers);

    pc->cached = 0;
    pc->connection = NULL;

  
