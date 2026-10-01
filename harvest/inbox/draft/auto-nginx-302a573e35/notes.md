# auto-nginx-302a573e35

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #898 (https://github.com/nginx/nginx/pull/898)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 8（原始 PR diff 行 5076；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -116,6 +116,10 @@ static ngx_int_t ngx_http_upstream_process_set_cookie(ngx_http_request_t *r,
 static ngx_int_t
     ngx_http_upstream_process_cache_control(ngx_http_request_t *r,
     ngx_table_elt_t *h, ngx_uint_t offset);
+#if (NGX_HTTP_CACHE)
+static ngx_int_t ngx_http_upstream_process_delta_seconds(u_char *p,
+    u_char *last);
+#endif
 static ngx_int_t ngx_http_upstream_ignore_header_line(ngx_http_request_t *r,
     ngx_table_elt_t *h, ngx_uint_t offset);
 static ngx_int_t ngx_http_upstream_process_expires(ngx_http_request_t *r,
@@ -5066,18 +5070,9 @@ ngx_http_upstream_process_cache_control(ngx_http_request_t *r,
     }
 
     if (p) {
-        n = 0;
-
-        for (p += offset; p < last; p++) {
-            if (*p == ',' || *p == ';' || *p == ' ') {
-                break;
-            }
-
-            if (*p >= '0' && *p <= '9') {
-                n = n * 10 + (*p - '0');
-                continue;
-            }
+        n = ngx_http_upstream_process_delta_seconds(p + offset, last);
 
+        if (n == NGX_ERROR) {
             u->cacheable = 0;
             return NGX_OK;
         }
@@ -5087,7 +5082,8 @@ ngx_http_upstream_process_cache_control(ngx_http_request_t *r,
             return NGX_OK;
         }
 
-        r->cache->valid_sec = ngx_time() + n;
+        r->cache->valid_sec = ngx_min((ngx_uint_t) ngx_time() + n,
+                                      NGX_MAX_INT_T_VALUE);
         u->headers_in.expired = 0;
     }
 
@@ -5097,18 +5093,9 @@ ngx_http_upstream_process_cache_control(ngx_http_request_t *r,
                          23 - 1);
 
     if (p) {
-        n = 0;
-
-        for (p += 23; p < last; p++) {
-            if (*p == ',' || *p == ';' || *p == ' ') {
-                break;
-            }
-
-            if (*p >= '0' && *p <= '9') {
-                n = n * 10 + (*p - '0');
-                continue;
-            }
+        n = ngx_http_upstream_process_delta_seconds(p + 23, last);
 
+        if (n == NGX_ERROR) {
             u->cacheable = 0;
             return NGX_OK;
         }
@@ -5120,18 +5107,9 @@ ngx_http_upstream_process_cache_control(ngx_http_request_t *r,
     p = ngx_strlcasestrn(start, last, (u_char *) "stale-if-error=", 15 - 1);
 
     if (p) {
-        n = 0;
-
-        for (p += 15; p < last; p++) {
-            if (*p == ',' || *p == ';' || *p == ' ') {
-                break;
-            }
-
-            if (*p >= '0' && *p <= '9') {
-                n = n * 10 + (*p - '0');
-                continue;
-            }
+        n = ngx_http_upstream_process_delta_seconds(p + 15, last);
 
+        if (n == NGX_ERROR) {
             u->cacheable = 0;
             return NGX_OK;
         }
@@ -5145,6 +5123,41 @@ ngx_http_upstream_process_cache_control(ngx_http_request_t *r,
 }
 
 
+#if (NGX_HTTP_CACHE)
+
+static ngx_int_t
+ngx_http_upstream_process_delta_seconds(u_char *p, u_char *last)
+{
+    ngx_int_t  n, cutoff, cutlim;
+
+    cutoff = NGX_MAX_INT_T_VALUE / 10;
+    cutlim = NGX_MAX_INT_T_VALUE % 10;
+
+    n = 0;
+
+    for ( /* void */ ; p < last; p++) {
+        if (*p == ',' || *p == ';' || *p == ' ') {
+            break;
+        }
+
+        if (*p < '0' || *p > '9') {
+            return NGX_ERROR;
+        }
+
+        if (n >= cutoff && (n > cutoff || *p - '0' > cutlim)) {
+            n = NGX_MAX_INT_T_VALUE;
+            break;
+        }
+
+        n = n * 10 + (*p - '0');
+    }
+
+    return n;
+}
+
+#endif
+
+
 static ngx_int_t
 ngx_http_upstream_process_expires(ngx_http_request_t *r, ngx_table_elt_t *h,
     ngx_uint_t offset)
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-302a573e35` → 本草稿移入 `cases/defect/auto-nginx-302a573e35/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
