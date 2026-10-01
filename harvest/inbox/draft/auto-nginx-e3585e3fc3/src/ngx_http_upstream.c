// AUTO-DRAFT from nginx/nginx PR #750
return NGX_ERROR;
    }
  // <<< BUG ANCHOR
    if (rc == NGX_AGAIN) {
        rc = NGX_HTTP_UPSTREAM_INVALID_HEADER;
    }

            continue;
        }

        if (rc == NGX_OK
            && u->headers_in.status_n == NGX_HTTP_EARLY_HINTS)
        {
            rc = ngx_http_upstream_process_early_hints(r, u);

            if (rc == NGX_OK) {
        }
    }

    if (u->reinit_request(r) != NGX_OK) {
        return NGX_ERROR;
    }

    ngx_http_clean_header(r);

    ngx_memzero(&u->headers_in, sizeof(ngx_http_upstream_headers_in_t));
