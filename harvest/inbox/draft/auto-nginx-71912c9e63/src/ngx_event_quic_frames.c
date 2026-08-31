// AUTO-DRAFT from nginx/nginx PR #1592
case NGX_QUIC_FT_ACK:
    case NGX_QUIC_FT_ACK_ECN:
  // <<< BUG ANCHOR
        p = ngx_slprintf(p, last, "ACK n:%ui delay:%uL ",
                         f->u.ack.range_count, f->u.ack.delay);

        if (f->data) {

    case NGX_QUIC_FT_CONNECTION_CLOSE:
    case NGX_QUIC_FT_CONNECTION_CLOSE_APP:
        p = ngx_slprintf(p, last, "CONNECTION_CLOSE%s err:%ui",
                         f->type == NGX_QUIC_FT_CONNECTION_CLOSE ? "" : "_APP",
                         f->u.close.error_code);

        }

        if (f->type == NGX_QUIC_FT_CONNECTION_CLOSE) {
            p = ngx_slprintf(p, last, " ft:%ui", f->u.close.frame_type);
        }

        break;
