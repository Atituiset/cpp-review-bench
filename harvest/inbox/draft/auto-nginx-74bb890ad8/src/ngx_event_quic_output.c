// AUTO-DRAFT from nginx/nginx PR #655
size_t len, struct sockaddr *sockaddr, socklen_t socklen, size_t segment);
#endif
static ssize_t ngx_quic_output_packet(ngx_connection_t *c,
    ngx_quic_send_ctx_t *ctx, u_char *data, size_t max, size_t min);  // <<< BUG ANCHOR
static void ngx_quic_init_packet(ngx_connection_t *c, ngx_quic_send_ctx_t *ctx,
    ngx_quic_header_t *pkt, ngx_quic_path_t *path);
static ngx_uint_t ngx_quic_get_padding_level(ngx_connection_t *c);
    ngx_memzero(preserved_pnum, sizeof(preserved_pnum));
#endif

    while (cg->in_flight < cg->window) {

        p = dst;

        len = ngx_quic_path_limit(c, path, path->mtu);
                return NGX_OK;
            }

            n = ngx_quic_output_packet(c, ctx, p, len, min);
            if (n == NGX_ERROR) {
                return NGX_ERROR;
            }
        ngx_quic_commit_send(c);

        path->sent += len;
    }

    return NGX_OK;
}

        bytes += f->len;

        if (bytes > len * 3) {
            /* require at least ~3 full packets to batch */
            return 1;

        if (len && cg->in_flight + (p - dst) < cg->window) {

            n = ngx_quic_output_packet(c, ctx, p, len, len);
            if (n == NGX_ERROR) {
                return NGX_ERROR;
            }

static ssize_t
ngx_quic_output_packet(ngx_connection_t *c, ngx_quic_send_ctx_t *ctx,
    u_char *data, size_t max, size_t min)
{
    size_t                  len, pad, min_payload, max_payload;
    u_char                 *p;
    {
        f = ngx_queue_data(q, ngx_quic_frame_t, queue);

        if (len >= max_payload) {
            break;
        }
