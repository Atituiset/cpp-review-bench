# auto-nginx-eed03b3c20

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1359 (https://github.com/nginx/nginx/pull/1359)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -241,6 +241,14 @@ ngx_http_v2_header_filter(ngx_http_request_t *r)
     }
 
     if (r->headers_out.content_type.len) {
+
+        if (r->headers_out.content_type.len > NGX_HTTP_V2_MAX_FIELD) {
+            ngx_log_error(NGX_LOG_CRIT, fc->log, 0,
+                          "too long response header value: "
+                          "\"Content-Type: %V\"", &r->headers_out.content_type);
+            return NGX_ERROR;
+        }
+
         len += 1 + NGX_HTTP_V2_INT_OCTETS + r->headers_out.content_type.len;
 
         if (r->headers_out.content_type_len == r->headers_out.content_type.len
@@ -264,6 +272,13 @@ ngx_http_v2_header_filter(ngx_http_request_t *r)
 
     if (r->headers_out.location && r->headers_out.location->value.len) {
 
+        if (r->headers_out.location->value.len > NGX_HTTP_V2_MAX_FIELD) {
+            ngx_log_error(NGX_LOG_CRIT, fc->log, 0,
+                          "too long response header value: \"Location: %V\"",
+                          &r->headers_out.location->value);
+            return NGX_ERROR;
+        }
+
         if (r->headers_out.location->value.data[0] == '/'
             && clcf->absolute_redirect)
         {
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-eed03b3c20` → 本草稿移入 `cases/defect/auto-nginx-eed03b3c20/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
