# auto-nginx-62034b927d

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #966 (https://github.com/nginx/nginx/pull/966)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 1898；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -931,7 +931,7 @@ ngx_http_ssl_servername(ngx_ssl_conn_t *ssl_conn, int *ad, void *arg)
         goto done;
     }
 
-    rc = ngx_http_validate_host(&host, c->pool, 1);
+    rc = ngx_http_validate_host(&host, NULL, c->pool, 1);
 
     if (rc == NGX_ERROR) {
         goto error;
@@ -1107,6 +1107,7 @@ ngx_http_process_request_line(ngx_event_t *rev)
     ssize_t              n;
     ngx_int_t            rc, rv;
     ngx_str_t            host;
+    in_port_t            port;
     ngx_connection_t    *c;
     ngx_http_request_t  *r;
 
@@ -1169,7 +1170,7 @@ ngx_http_process_request_line(ngx_event_t *rev)
                 host.len = r->host_end - r->host_start;
                 host.data = r->host_start;
 
-                rc = ngx_http_validate_host(&host, r->pool, 0);
+                rc = ngx_http_validate_host(&host, &port, r->pool, 0);
 
                 if (rc == NGX_DECLINED) {
                     ngx_log_error(NGX_LOG_INFO, c->log, 0,
@@ -1188,6 +1189,7 @@ ngx_http_process_request_line(ngx_event_t *rev)
                 }
 
                 r->headers_in.server = host;
+                r->port = port;
             }
 
             if (r->http_version < NGX_HTTP_VERSION_10) {
@@ -1846,9 +1848,9 @@ static ngx_int_t
 ngx_http_process_host(ngx_http_request_t *r, ngx_table_elt_t *h,
     ngx_uint_t offset)
 {
-    u_char     *p;
-    ngx_int_t   rc;
-    ngx_str_t   host;
+    ngx_int_t  rc;
+    ngx_str_t  host;
+    in_port_t  port;
 
     if (r->headers_in.host) {
         ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
@@ -1865,7 +1867,7 @@ ngx_http_process_host(ngx_http_request_t *r, ngx_table_elt_t *h,
 
     host = h->value;
 
-    rc = ngx_http_validate_host(&host, r->pool, 0);
+    rc = ngx_http_validate_host(&host, &port, r->pool, 0);
 
     if (rc == NGX_DECLINED) {
         ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
@@ -1888,17 +1890,7 @@ ngx_http_process_host(ngx_http_request_t *r, ngx_table_elt_t *h,
     }
 
     r->headers_in.server = host;
-
-    p = ngx_strlchr(h->value.data + host.len,
-                    h->value.data + h->value.len, ':');
-
-    if (p) {
-        rc = ngx_atoi(p + 1, h->value.data + h->value.len - p - 1);
-
-        if (rc > 0 && rc < 65536) {
-            r->port = rc;
-        }
-    }
+    r->port = port;
 
     return NGX_OK;
 }
@@ -2182,74 +2174,176 @@ ngx_http_process_request(ngx_http_request_t *r)
 
 
 ngx_int_t
-ngx_http_validate_host(ngx_str_t *host, ngx_pool_t *pool, ngx_uint_t alloc)
+ngx_http_validate_host(ngx_str_t *host, in_port_t *portp, ngx_pool_t *pool,
+    ngx_uint_t alloc)
 {
-    u_char  *h, ch;
-    size_t   i, dot_pos, host_len;
+    u_char     *h, ch;
+    size_t      i, dot_pos, host_len;
+    ngx_int_t   port;
 
     enum {
-        sw_usual = 0,
-        sw_literal,
-        sw_rest
+        sw_host_start = 0,
+        sw_host,
+        sw_host_ip_literal,
+        sw_host_end,
+        sw_port,
     } state;
 
     dot_pos = host->len;
     host_len = host->len;
+    port = 0;
 
     h = host->data;
 
-    state = sw_usual;
+    state = sw_host_start;
 
     for (i = 0; i < host->len; i++) {
         ch = h[i];
 
-        switch (ch) {
+        switch (state) {
 
-        case '.':
-            if (dot_pos == i - 1) {
-                return NGX_DECLINED;
-            }
-            dot_pos = i;
-            break;
+        case sw_host_start:
 
-        case ':':
-            if (state == sw_usual) {
-                host_len = i;
-                state = sw_rest;
+            if (ch == '[') {
+                state = sw_host_ip_literal;
+                break;
             }
-            break;
 
-        case '[':
-            if (i == 0) {
-                state = sw_literal;
-            }
-            break;
+            state = sw_host;
 
-        case ']':
-            if (state == sw_literal) {
-                host_len = i + 1;
-                state = sw_rest;
+            /* fall through */
+
+        case sw_host:
+
+            if (ch >
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-62034b927d` → 本草稿移入 `cases/defect/auto-nginx-62034b927d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
