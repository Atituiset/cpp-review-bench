// AUTO-DRAFT from nginx/nginx PR #1248
sr->method = NGX_HTTP_GET;
    sr->http_version = r->http_version;

    sr->request_line = r->request_line;
    sr->uri = *uri;
