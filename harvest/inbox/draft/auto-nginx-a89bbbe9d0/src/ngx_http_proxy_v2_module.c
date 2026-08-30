// AUTO-DRAFT from nginx/nginx PR #1314
f = (ngx_http_proxy_v2_frame_t *) headers_frame;
            f->flags |= NGX_HTTP_V2_END_STREAM_FLAG;
        }
    }

    u->output.output_filter = ngx_http_proxy_v2_body_output_filter;
