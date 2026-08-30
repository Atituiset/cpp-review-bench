// AUTO-DRAFT from nginx/nginx PR #966
case sw_host_end:
  // <<< BUG ANCHOR
            r->host_end = p;

            if (r->method == NGX_HTTP_CONNECT) {
                if (ch == ':') {
                    state = sw_port;
                    break;
                }

                return NGX_HTTP_PARSE_INVALID_REQUEST;
            }

            switch (ch) {
            case ':':
                state = sw_port;
                break;
            case '/':
                r->uri_start = p;
                state = sw_after_slash_in_uri;

        case sw_port:
            if (ch >= '0' && ch <= '9') {
                if (r->port >= 6553 && (r->port > 6553 || (ch - '0') > 5)) {
                    return NGX_HTTP_PARSE_INVALID_REQUEST;
                }

                r->port = r->port * 10 + (ch - '0');
                break;
            }

            if (r->method == NGX_HTTP_CONNECT) {
                if (ch == ' ') {
                    state = sw_http_09;
