# auto-nginx-4f3227bb9f

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1307 (https://github.com/nginx/nginx/pull/1307)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -47,6 +47,9 @@ static ngx_int_t ngx_http_dav_mkcol_handler(ngx_http_request_t *r,
     ngx_http_dav_loc_conf_t *dlcf);
 
 static ngx_int_t ngx_http_dav_copy_move_handler(ngx_http_request_t *r);
+static void ngx_http_dav_merge_slashes(ngx_str_t *path);
+static ngx_int_t ngx_http_dav_validate_paths(ngx_http_request_t *r,
+    ngx_str_t *src, ngx_str_t *dst, ngx_uint_t slash, ngx_table_elt_t *dest);
 static ngx_int_t ngx_http_dav_copy_dir(ngx_tree_ctx_t *ctx, ngx_str_t *path);
 static ngx_int_t ngx_http_dav_copy_dir_time(ngx_tree_ctx_t *ctx,
     ngx_str_t *path);
@@ -719,6 +722,9 @@ ngx_http_dav_copy_move_handler(ngx_http_request_t *r)
 
     r->uri = uri;
 
+    ngx_http_dav_merge_slashes(&path);
+    ngx_http_dav_merge_slashes(&copy.path);
+
     copy.path.len--;  /* omit "\0" */
 
     if (copy.path.data[copy.path.len - 1] == '/') {
@@ -733,6 +739,12 @@ ngx_http_dav_copy_move_handler(ngx_http_request_t *r)
     ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                    "http copy to: \"%s\"", copy.path.data);
 
+    if (ngx_http_dav_validate_paths(r, &path, &copy.path, slash, dest)
+        != NGX_OK)
+    {
+        return NGX_HTTP_FORBIDDEN;
+    }
+
     if (ngx_link_info(copy.path.data, &fi) == NGX_FILE_ERROR) {
         err = ngx_errno;
 
@@ -870,6 +882,65 @@ ngx_http_dav_copy_move_handler(ngx_http_request_t *r)
 }
 
 
+static void
+ngx_http_dav_merge_slashes(ngx_str_t *path)
+{
+    u_char  *p, *q;
+
+    p = path->data;
+    q = path->data;
+
+    while (*p) {
+        *q++ = *p;
+
+        if (*p++ == '/') {
+            while (*p == '/') {
+                p++;
+            }
+        }
+    }
+
+    *q++ = '\0';
+    path->len = q - path->data;
+}
+
+
+static ngx_int_t
+ngx_http_dav_validate_paths(ngx_http_request_t *r, ngx_str_t *src,
+    ngx_str_t *dst, ngx_uint_t slash, ngx_table_elt_t *dest)
+{
+    size_t  len;
+
+    len = src->len - 1;
+
+    if (len > 0 && src->data[len - 1] == '/') {
+        len--;
+    }
+
+    if (len == dst->len && ngx_strncmp(src->data, dst->data, len) == 0) {
+        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
+                      "both URI \"%V\" and \"Destination\" URI \"%V\" "
+                      "point to the same location",
+                      &r->uri, &dest->value);
+        return NGX_HTTP_FORBIDDEN;
+    }
+
+    if (slash
+        && ngx_strncmp(src->data, dst->data, ngx_min(len, dst->len)) == 0
+        && (len < dst->len
+            ? dst->data[len] == '/'
+            : src->data[dst->len] == '/'))
+    {
+        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
+                      "\"%V\" could not be %Ved to collection \"%V\"",
+                      &r->uri, &r->method_name, &dest->value);
+        return NGX_HTTP_FORBIDDEN;
+    }
+
+    return NGX_OK;
+}
+
+
 static ngx_int_t
 ngx_http_dav_copy_dir(ngx_tree_ctx_t *ctx, ngx_str_t *path)
 {
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-4f3227bb9f` → 本草稿移入 `cases/defect/auto-nginx-4f3227bb9f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
