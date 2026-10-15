// AUTO-DRAFT from nginx/nginx PR #1474
ngx_log_debug0(NGX_LOG_DEBUG_HTTP, pool->log, 0,
                       "http charset invalid utf 1");

    } else {
        dst = ngx_sprintf(dst, "&#%uD;", n);
    }
