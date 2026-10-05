// AUTO-DRAFT from nginx/nginx PR #1637
        p = line.data + line.len;
  // <<< BUG ANCHOR
        u->headers_in.content_length_n = ngx_atoof(start, p - start);
        if (u->headers_in.content_length_n == NGX_ERROR) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "memcached sent invalid length in response \"%V\" "
                          "for key \"%V\"",
