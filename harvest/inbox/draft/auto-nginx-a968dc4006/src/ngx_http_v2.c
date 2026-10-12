// AUTO-DRAFT from nginx/nginx PR #1441
return ngx_http_v2_connection_error(h2c, NGX_HTTP_V2_SIZE_ERROR);
    }
  // <<< BUG ANCHOR
    return ngx_http_v2_state_settings_params(h2c, pos, end);
}

ngx_http_v2_state_settings_params(ngx_http_v2_connection_t *h2c, u_char *pos,
    u_char *end)
{
    ssize_t                   window_delta;
    ngx_uint_t                id, value;
    ngx_http_v2_out_frame_t  *frame;

    window_delta = 0;

    while (h2c->state.length) {
        if (end - pos < NGX_HTTP_V2_SETTINGS_PARAM_SIZE) {
            return ngx_http_v2_state_save(h2c, pos, end,
                                                  NGX_HTTP_V2_FLOW_CTRL_ERROR);
            }

            window_delta = value - h2c->init_window;
            break;

        case NGX_HTTP_V2_MAX_FRAME_SIZE_SETTING:

    ngx_http_v2_queue_ordered_frame(h2c, frame);

    if (window_delta) {
        h2c->init_window += window_delta;

        if (ngx_http_v2_adjust_windows(h2c, window_delta) != NGX_OK) {
            return ngx_http_v2_connection_error(h2c,
                                                NGX_HTTP_V2_INTERNAL_ERROR);
        }
    }

    return ngx_http_v2_state_complete(h2c, pos, end);
