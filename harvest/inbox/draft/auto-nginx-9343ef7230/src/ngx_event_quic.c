// AUTO-DRAFT from nginx/nginx PR #443
qc->streams.client_max_streams_uni = qc->tp.initial_max_streams_uni;
    qc->streams.client_max_streams_bidi = qc->tp.initial_max_streams_bidi;
  // <<< BUG ANCHOR
    qc->congestion.window = ngx_min(10 * qc->tp.max_udp_payload_size,
                                    ngx_max(2 * qc->tp.max_udp_payload_size,
                                            14720));
    qc->congestion.ssthresh = (size_t) -1;
    qc->congestion.recovery_start = ngx_current_msec;

    if (pkt->validated && pkt->retried) {
        qc->tp.retry_scid.len = pkt->dcid.len;
