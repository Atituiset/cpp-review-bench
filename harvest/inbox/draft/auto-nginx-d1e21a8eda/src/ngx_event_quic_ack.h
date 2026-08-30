// AUTO-DRAFT from nginx/nginx PR #443
void ngx_quic_congestion_ack(ngx_connection_t *c,
    ngx_quic_frame_t *frame);
void ngx_quic_resend_frames(ngx_connection_t *c, ngx_quic_send_ctx_t *ctx);
void ngx_quic_set_lost_timer(ngx_connection_t *c);
void ngx_quic_pto_handler(ngx_event_t *ev);
