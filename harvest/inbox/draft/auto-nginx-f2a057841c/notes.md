# auto-nginx-f2a057841c

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #803 (https://github.com/nginx/nginx/pull/803)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 9（原始 PR diff 行 3527；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -158,6 +158,7 @@ static ngx_int_t ngx_http_v2_construct_request_line(ngx_http_request_t *r);
 static ngx_int_t ngx_http_v2_cookie(ngx_http_request_t *r,
     ngx_http_v2_header_t *header);
 static ngx_int_t ngx_http_v2_construct_cookie_header(ngx_http_request_t *r);
+static ngx_int_t ngx_http_v2_construct_host_header(ngx_http_request_t *r);
 static void ngx_http_v2_run_request(ngx_http_request_t *r);
 static ngx_int_t ngx_http_v2_process_request_body(ngx_http_request_t *r,
     u_char *pos, size_t size, ngx_uint_t last, ngx_uint_t flush);
@@ -3517,44 +3518,40 @@ ngx_http_v2_parse_scheme(ngx_http_request_t *r, ngx_str_t *value)
 static ngx_int_t
 ngx_http_v2_parse_authority(ngx_http_request_t *r, ngx_str_t *value)
 {
-    ngx_table_elt_t            *h;
-    ngx_http_header_t          *hh;
-    ngx_http_core_main_conf_t  *cmcf;
+    ngx_int_t  rc;
 
-    static ngx_str_t host = ngx_string("host");
-
-    h = ngx_list_push(&r->headers_in.headers);
-    if (h == NULL) {
-        return NGX_ERROR;
+    if (r->host_start) {
+        ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
+                      "client sent duplicate \":authority\" header");
+        return NGX_DECLINED;
     }
 
-    h->hash = ngx_hash(ngx_hash(ngx_hash('h', 'o'), 's'), 't');
-
-    h->key.len = host.len;
-    h->key.data = host.data;
-
-    h->value.len = value->len;
-    h->value.data = value->data;
+    r->host_start = value->data;
+    r->host_end = value->data + value->len;
 
-    h->lowcase_key = host.data;
+    rc = ngx_http_validate_host(value, r->pool, 0);
 
-    cmcf = ngx_http_get_module_main_conf(r, ngx_http_core_module);
-
-    hh = ngx_hash_find(&cmcf->headers_in_hash, h->hash,
-                       h->lowcase_key, h->key.len);
+    if (rc == NGX_DECLINED) {
+        ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
+                      "client sent invalid \":authority\" header");
+        return NGX_DECLINED;
+    }
 
-    if (hh == NULL) {
-        return NGX_ERROR;
+    if (rc == NGX_ERROR) {
+        ngx_http_v2_close_stream(r->stream, NGX_HTTP_INTERNAL_SERVER_ERROR);
+        return NGX_ABORT;
     }
 
-    if (hh->handler(r, h, hh->offset) != NGX_OK) {
+    if (ngx_http_set_virtual_server(r, value) == NGX_ERROR) {
         /*
          * request has been finalized already
-         * in ngx_http_process_host()
+         * in ngx_http_set_virtual_server()
          */
         return NGX_ABORT;
     }
 
+    r->headers_in.server = *value;
+
     return NGX_OK;
 }
 
@@ -3725,7 +3722,54 @@ ngx_http_v2_construct_cookie_header(ngx_http_request_t *r)
     if (hh->handler(r, h, hh->offset) != NGX_OK) {
         /*
          * request has been finalized already
-         * in ngx_http_process_multi_header_lines()
+         * in ngx_http_process_header_line()
+         */
+        return NGX_ERROR;
+    }
+
+    return NGX_OK;
+}
+
+
+static ngx_int_t
+ngx_http_v2_construct_host_header(ngx_http_request_t *r)
+{
+    ngx_table_elt_t            *h;
+    ngx_http_header_t          *hh;
+    ngx_http_core_main_conf_t  *cmcf;
+
+    static ngx_str_t host = ngx_string("host");
+
+    h = ngx_list_push(&r->headers_in.headers);
+    if (h == NULL) {
+        ngx_http_v2_close_stream(r->stream, NGX_HTTP_INTERNAL_SERVER_ERROR);
+        return NGX_ERROR;
+    }
+
+    h->hash = ngx_hash(ngx_hash(ngx_hash('h', 'o'), 's'), 't');
+
+    h->key.len = host.len;
+    h->key.data = host.data;
+
+    h->value.len = r->host_end - r->host_start;
+    h->value.data = r->host_start;
+
+    h->lowcase_key = host.data;
+
+    cmcf = ngx_http_get_module_main_conf(r, ngx_http_core_module);
+
+    hh = ngx_hash_find(&cmcf->headers_in_hash, h->hash,
+                       h->lowcase_key, h->key.len);
+
+    if (hh == NULL) {
+        ngx_http_v2_close_stream(r->stream, NGX_HTTP_INTERNAL_SERVER_ERROR);
+        return NGX_ERROR;
+    }
+
+    if (hh->handler(r, h, hh->offset) != NGX_OK) {
+        /*
+         * request has been finalized already
+         * in 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-f2a057841c` → 本草稿移入 `cases/defect/auto-nginx-f2a057841c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
