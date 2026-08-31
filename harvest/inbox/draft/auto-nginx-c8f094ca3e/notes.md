# auto-nginx-c8f094ca3e

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1593 (https://github.com/nginx/nginx/pull/1593)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 812；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -25,7 +25,7 @@ typedef struct {
     ngx_array_t               *headers_source;
 
     ngx_str_t                  host;
-    ngx_uint_t                 host_set;
+    ngx_http_complex_value_t  *host_value;
 
     ngx_array_t               *grpc_lengths;
     ngx_array_t               *grpc_values;
@@ -739,6 +739,7 @@ ngx_http_grpc_create_request(ngx_http_request_t *r)
                                   key_len, val_len, uri_len;
     uintptr_t                     escape;
     ngx_buf_t                    *b;
+    ngx_str_t                     host;
     ngx_uint_t                    i, next;
     ngx_chain_t                  *cl, *body;
     ngx_list_part_t              *part;
@@ -809,18 +810,28 @@ ngx_http_grpc_create_request(ngx_http_request_t *r)
 
     /* :authority header */
 
-    if (!glcf->host_set) {
-        if (ctx->host.len > NGX_HTTP_V2_MAX_FIELD) {
-            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
-                          "too long http2 host: \"%V\"", &ctx->host);
-            return NGX_ERROR;
-        }
+    host.len = 0;
 
-        len += 1 + NGX_HTTP_V2_INT_OCTETS + ctx->host.len;
+    if (glcf->host_value
+        && ngx_http_complex_value(r, glcf->host_value, &host) != NGX_OK)
+    {
+        return NGX_ERROR;
+    }
 
-        if (tmp_len < ctx->host.len) {
-            tmp_len = ctx->host.len;
-        }
+    if (host.len == 0) {
+        host = ctx->host;
+    }
+
+    if (host.len > NGX_HTTP_V2_MAX_FIELD) {
+        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
+                      "too long http2 host: \"%V\"", &host);
+        return NGX_ERROR;
+    }
+
+    len += 1 + NGX_HTTP_V2_INT_OCTETS + host.len;
+
+    if (tmp_len < host.len) {
+        tmp_len = host.len;
     }
 
     /* other headers */
@@ -1051,14 +1062,11 @@ ngx_http_grpc_create_request(ngx_http_request_t *r)
                        "grpc header: \":path: %V\"", &r->uri);
     }
 
-    if (!glcf->host_set) {
-        *b->last++ = ngx_http_v2_inc_indexed(NGX_HTTP_V2_AUTHORITY_INDEX);
-        b->last = ngx_http_v2_write_value(b->last, ctx->host.data,
-                                          ctx->host.len, tmp);
+    *b->last++ = ngx_http_v2_inc_indexed(NGX_HTTP_V2_AUTHORITY_INDEX);
+    b->last = ngx_http_v2_write_value(b->last, host.data, host.len, tmp);
 
-        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
-                       "grpc header: \":authority: %V\"", &ctx->host);
-    }
+    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
+                   "grpc header: \":authority: %V\"", &host);
 
     ngx_memzero(&e, sizeof(ngx_http_script_engine_t));
 
@@ -4514,7 +4522,7 @@ ngx_http_grpc_create_loc_conf(ngx_conf_t *cf)
      *     conf->headers.values = NULL;
      *     conf->headers.hash = { NULL, 0 };
      *     conf->host = { 0, NULL };
-     *     conf->host_set = 0;
+     *     conf->host_value = NULL;
      *     conf->ssl = 0;
      *     conf->ssl_protocols = 0;
      *     conf->ssl_ciphers = { 0, NULL };
@@ -4722,7 +4730,7 @@ ngx_http_grpc_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
 
     if (conf->headers_source == prev->headers_source) {
         conf->headers = prev->headers;
-        conf->host_set = prev->host_set;
+        conf->host_value = prev->host_value;
     }
 
     rc = ngx_http_grpc_init_headers(cf, conf, &conf->headers,
@@ -4740,7 +4748,7 @@ ngx_http_grpc_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
         && conf->headers_source == prev->headers_source)
     {
         prev->headers = conf->headers;
-        prev->host_set = conf->host_set;
+        prev->host_value = conf->host_value;
     }
 
     return NGX_CONF_OK;
@@ -4751,16 +4759,17 @@ static ngx_int_t
 ngx_http_grpc_init_headers(ngx_conf_t *cf, ngx_http_grpc_loc_conf_t *conf,
     ngx_http_grpc_headers_t *headers, ngx_keyval_t *default_headers)
 {
-    u_char                       *p;
-    size_t                        size;
-    uintptr_t                    *code;
-
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-c8f094ca3e` → 本草稿移入 `cases/defect/auto-nginx-c8f094ca3e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
