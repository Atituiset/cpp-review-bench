// AUTO-DRAFT from nginx/nginx PR #353
* After receiving a 1-RTT packet, servers MUST discard
         * 0-RTT keys within a short time
         */
        ngx_quic_discard_ctx(c, ssl_encryption_early_data);  // <<< BUG ANCHOR
    }

    if (qc->closing) {
