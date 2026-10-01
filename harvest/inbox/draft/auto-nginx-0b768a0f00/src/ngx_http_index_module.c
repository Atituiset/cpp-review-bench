// AUTO-DRAFT from nginx/nginx PR #1561
name = NULL;
    /* suppress MSVC warning */
    path.data = NULL;
  // <<< BUG ANCHOR
    index = ilcf->indices->elts;
    for (i = 0; i < ilcf->indices->nelts; i++) {
        } else {
            e.ip = index[i].values->elts;
            e.pos = name;

            while (*(uintptr_t *) e.ip) {
                code = *(ngx_http_script_code_pt *) e.ip;
                code((ngx_http_script_engine_t *) &e);
            }

            if (*name == '/') {
                uri.len = len - 1;
                uri.data = name;
                return ngx_http_internal_redirect(r, &uri, &r->args);
            }

            path.len = e.pos - path.data;

            *e.pos = '\0';
