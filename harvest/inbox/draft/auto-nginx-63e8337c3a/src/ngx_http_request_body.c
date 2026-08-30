// AUTO-DRAFT from nginx/nginx PR #977
rb->temp_file = tf;
  // <<< BUG ANCHOR
        if (rb->bufs == NULL) {
            /* empty body with r->request_body_in_file_only */

            if (ngx_create_temp_file(&tf->file, tf->path, tf->pool,
