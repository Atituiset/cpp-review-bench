# auto-nginx-4840258d38

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1080 (https://github.com/nginx/nginx/pull/1080)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 10（原始 PR diff 行 2788；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -84,9 +84,6 @@ static ngx_int_t ngx_http_proxy_port_variable(ngx_http_request_t *r,
 static ngx_int_t
     ngx_http_proxy_add_x_forwarded_for_variable(ngx_http_request_t *r,
     ngx_http_variable_value_t *v, uintptr_t data);
-static ngx_int_t
-    ngx_http_proxy_internal_connection_variable(ngx_http_request_t *r,
-    ngx_http_variable_value_t *v, uintptr_t data);
 static ngx_int_t
     ngx_http_proxy_internal_body_length_variable(ngx_http_request_t *r,
     ngx_http_variable_value_t *v, uintptr_t data);
@@ -749,7 +746,7 @@ static char  ngx_http_proxy_version_11[] = " HTTP/1.1" CRLF;
 
 static ngx_keyval_t  ngx_http_proxy_headers[] = {
     { ngx_string("Host"), ngx_string("$proxy_internal_host") },
-    { ngx_string("Connection"), ngx_string("$proxy_internal_connection") },
+    { ngx_string("Connection"), ngx_string("") },
     { ngx_string("Content-Length"), ngx_string("$proxy_internal_body_length") },
     { ngx_string("Transfer-Encoding"), ngx_string("$proxy_internal_chunked") },
     { ngx_string("TE"), ngx_string("") },
@@ -777,7 +774,7 @@ static ngx_str_t  ngx_http_proxy_hide_headers[] = {
 
 static ngx_keyval_t  ngx_http_proxy_cache_headers[] = {
     { ngx_string("Host"), ngx_string("$proxy_internal_host") },
-    { ngx_string("Connection"), ngx_string("$proxy_internal_connection") },
+    { ngx_string("Connection"), ngx_string("") },
     { ngx_string("Content-Length"), ngx_string("$proxy_internal_body_length") },
     { ngx_string("Transfer-Encoding"), ngx_string("$proxy_internal_chunked") },
     { ngx_string("TE"), ngx_string("") },
@@ -816,10 +813,6 @@ static ngx_http_variable_t  ngx_http_proxy_vars[] = {
       ngx_http_proxy_host_variable, 1,
       NGX_HTTP_VAR_CHANGEABLE|NGX_HTTP_VAR_NOCACHEABLE|NGX_HTTP_VAR_NOHASH, 0 },
 
-    { ngx_string("proxy_internal_connection"), NULL,
-      ngx_http_proxy_internal_connection_variable, 0,
-      NGX_HTTP_VAR_NOCACHEABLE|NGX_HTTP_VAR_NOHASH, 0 },
-
     { ngx_string("proxy_internal_body_length"), NULL,
       ngx_http_proxy_internal_body_length_variable, 0,
       NGX_HTTP_VAR_NOCACHEABLE|NGX_HTTP_VAR_NOHASH, 0 },
@@ -2777,29 +2770,6 @@ ngx_http_proxy_add_x_forwarded_for_variable(ngx_http_request_t *r,
 }
 
 
-static ngx_int_t
-ngx_http_proxy_internal_connection_variable(ngx_http_request_t *r,
-    ngx_http_variable_value_t *v, uintptr_t data)
-{
-    ngx_http_proxy_ctx_t  *ctx;
-
-    ctx = ngx_http_get_module_ctx(r, ngx_http_proxy_module);
-
-    if (ctx == NULL || !ctx->legacy) {
-        v->not_found = 1;
-        return NGX_OK;
-    }
-
-    v->valid = 1;
-    v->no_cacheable = 0;
-    v->not_found = 0;
-
-    ngx_str_set(v, "close");
-
-    return NGX_OK;
-}
-
-
 static ngx_int_t
 ngx_http_proxy_internal_body_length_variable(ngx_http_request_t *r,
     ngx_http_variable_value_t *v, uintptr_t data)
@@ -4030,7 +4000,7 @@ ngx_http_proxy_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
     ngx_conf_merge_ptr_value(conf->cookie_flags, prev->cookie_flags, NULL);
 
     ngx_conf_merge_uint_value(conf->http_version, prev->http_version,
-                              NGX_HTTP_VERSION_10);
+                              NGX_HTTP_VERSION_11);
 
     ngx_conf_merge_uint_value(conf->headers_hash_max_size,
                               prev->headers_hash_max_size, 512);
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-4840258d38` → 本草稿移入 `cases/defect/auto-nginx-4840258d38/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
