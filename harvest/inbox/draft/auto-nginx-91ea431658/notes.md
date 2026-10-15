# auto-nginx-91ea431658

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1476 (https://github.com/nginx/nginx/pull/1476)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -156,6 +156,8 @@ static ngx_int_t ngx_http_upstream_rewrite_set_cookie(ngx_http_request_t *r,
     ngx_table_elt_t *h, ngx_uint_t offset);
 static ngx_int_t ngx_http_upstream_copy_allow_ranges(ngx_http_request_t *r,
     ngx_table_elt_t *h, ngx_uint_t offset);
+static ngx_int_t ngx_http_upstream_copy_upgrade(ngx_http_request_t *r,
+    ngx_table_elt_t *h, ngx_uint_t offset);
 
 static ngx_int_t ngx_http_upstream_add_variables(ngx_conf_t *cf);
 static ngx_int_t ngx_http_upstream_addr_variable(ngx_http_request_t *r,
@@ -278,6 +280,10 @@ static ngx_http_upstream_header_t  ngx_http_upstream_headers_in[] = {
                  ngx_http_upstream_copy_allow_ranges,
                  offsetof(ngx_http_headers_out_t, accept_ranges), 1 },
 
+    { ngx_string("Upgrade"),
+                 ngx_http_upstream_ignore_header_line, 0,
+                 ngx_http_upstream_copy_upgrade, 0, 0 },
+
     { ngx_string("Content-Range"),
                  ngx_http_upstream_ignore_header_line, 0,
                  ngx_http_upstream_copy_header_line,
@@ -5868,6 +5874,27 @@ ngx_http_upstream_copy_allow_ranges(ngx_http_request_t *r,
 }
 
 
+static ngx_int_t
+ngx_http_upstream_copy_upgrade(ngx_http_request_t *r, ngx_table_elt_t *h,
+    ngx_uint_t offset)
+{
+    ngx_table_elt_t  *ho;
+
+    if (r->http_version >= NGX_HTTP_VERSION_20) {
+        return NGX_OK;
+    }
+
+    ho = ngx_list_push(&r->headers_out.headers);
+    if (ho == NULL) {
+        return NGX_ERROR;
+    }
+
+    *ho = *h;
+
+    return NGX_OK;
+}
+
+
 static ngx_int_t
 ngx_http_upstream_add_variables(ngx_conf_t *cf)
 {
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-91ea431658` → 本草稿移入 `cases/defect/auto-nginx-91ea431658/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
