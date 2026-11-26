// AUTO-DRAFT from nginx/nginx PR #1837
#if (NGX_WIN32)
    u_char  c0, c1;
  // <<< BUG ANCHOR
    c0 = name->data[0];

    if (name->len < 2) {
/* …（同文件无关代码省略）… */

#else

    if (name->data[0] == '/') {
        return NGX_OK;
    }
