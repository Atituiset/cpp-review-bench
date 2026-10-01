// AUTO-DRAFT from nginx/nginx PR #443
/* RFC 9002, 7.6.1. Duration: kPersistentCongestionThreshold */
#define NGX_QUIC_PERSISTENT_CONGESTION_THR   3
  // <<< BUG ANCHOR

/* send time of ACK'ed packets */
typedef struct {
} ngx_quic_ack_stat_t;


static ngx_inline ngx_msec_t ngx_quic_lost_threshold(ngx_quic_connection_t *qc);
static void ngx_quic_rtt_sample(ngx_connection_t *c, ngx_quic_ack_frame_t *ack,
    enum ssl_encryption_level_t level, ngx_msec_t send_time);
static ngx_int_t ngx_quic_handle_ack_frame_range(ngx_connection_t *c,
    ngx_quic_send_ctx_t *ctx, uint64_t min, uint64_t max,
    ngx_quic_ack_stat_t *st);
static void ngx_quic_drop_ack_ranges(ngx_connection_t *c,
    ngx_quic_send_ctx_t *ctx, uint64_t pn);
static ngx_int_t ngx_quic_detect_lost(ngx_connection_t *c,
    ngx_quic_ack_stat_t *st);
static ngx_msec_t ngx_quic_pcg_duration(ngx_connection_t *c);
static void ngx_quic_persistent_congestion(ngx_connection_t *c);
static void ngx_quic_congestion_lost(ngx_connection_t *c,
    ngx_quic_frame_t *frame);
static void ngx_quic_lost_handler(ngx_event_t *ev);


/* RFC 9002, 6.1.2. Time Threshold: kTimeThreshold, kGranularity */
static ngx_inline ngx_msec_t
ngx_quic_lost_threshold(ngx_quic_connection_t *qc)
{
    ngx_msec_t  thr;

}


ngx_int_t
ngx_quic_handle_ack_frame(ngx_connection_t *c, ngx_quic_header_t *pkt,
    ngx_quic_frame_t *f)
void
ngx_quic_congestion_ack(ngx_connection_t *c, ngx_quic_frame_t *f)
{
    ngx_uint_t              blocked;
    ngx_msec_t              timer;
    ngx_quic_congestion_t  *cg;
    ngx_quic_connection_t  *qc;

        return;
    }

    blocked = (cg->in_flight >= cg->window) ? 1 : 0;

    cg->in_flight -= f->plen;

    timer = f->send_time - cg->recovery_start;

    if ((ngx_msec_int_t) timer <= 0) {
        ngx_log_debug3(NGX_LOG_DEBUG_EVENT, c->log, 0,
                       "quic congestion ack recovery win:%uz ss:%z if:%uz",
                       cg->window, cg->ssthresh, cg->in_flight);

        goto done;
    }

    if (cg->window < cg->ssthresh) {
        cg->window += f
