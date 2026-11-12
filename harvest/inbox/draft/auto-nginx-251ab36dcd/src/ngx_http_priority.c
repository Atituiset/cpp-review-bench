// AUTO-DRAFT from nginx/nginx PR #1814
#define NGX_HTTP_PRIORITY_INTEGER  0
/* …（同文件无关代码省略）… */
  // <<< BUG ANCHOR
    n = ngx_atoi(start, last - start);
    if (n == NGX_ERROR) {
        return NGX_ERROR;
    }

    *type = NGX_HTTP_PRIORITY_INTEGER;
