// AUTO-DRAFT from nginx/nginx PR #1601
e->pos = e->buf.data;
    e->end = e->buf.data + len;
}
  // <<< BUG ANCHOR

void
ngx_http_script_complex_value_end_code(ngx_http_script_engine_t *e)
{
    e->ip += sizeof(ngx_http_script_complex_value_end_code_t);

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, e->request->connection->log, 0,
                   "http script complex value end");

    e->sp->len = e->pos - e->buf.data;
    e->sp->data = e->buf.data;
    e->sp++;
}
