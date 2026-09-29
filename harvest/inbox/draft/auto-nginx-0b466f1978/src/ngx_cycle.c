// AUTO-DRAFT from nginx/nginx PR #1082
ngx_cpystrn(cycle->conf_file.data, old_cycle->conf_file.data,
                old_cycle->conf_file.len + 1);
  // <<< BUG ANCHOR
    cycle->conf_param.len = old_cycle->conf_param.len;
    cycle->conf_param.data = ngx_pstrdup(pool, &old_cycle->conf_param);
    if (cycle->conf_param.data == NULL) {
        ngx_destroy_pool(pool);
        return NULL;
    }
