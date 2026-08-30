// AUTO-DRAFT from nginx/nginx PR #1592
} else {
                    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                       "too long parameter \"%*s...\" started",
                                       10, start);  // <<< BUG ANCHOR
                    return NGX_ERROR;
                }
