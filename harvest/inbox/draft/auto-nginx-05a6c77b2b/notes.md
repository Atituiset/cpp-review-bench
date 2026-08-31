# auto-nginx-05a6c77b2b

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #747 (https://github.com/nginx/nginx/pull/747)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 478；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -465,7 +465,7 @@ ngx_http_xslt_sax_error(void *data, const char *msg, ...)
 {
     xmlParserCtxtPtr ctxt = data;
 
-    size_t                       n;
+    int                          n;
     va_list                      args;
     ngx_http_xslt_filter_ctx_t  *ctx;
     u_char                       buf[NGX_MAX_ERROR_STR];
@@ -475,13 +475,23 @@ ngx_http_xslt_sax_error(void *data, const char *msg, ...)
     buf[0] = '\0';
 
     va_start(args, msg);
-    n = (size_t) vsnprintf((char *) buf, NGX_MAX_ERROR_STR, msg, args);
+    n = vsnprintf((char *) buf, NGX_MAX_ERROR_STR, msg, args);
     va_end(args);
 
+    if (n <= 0) {
+        ngx_log_error(NGX_LOG_ERR, ctx->request->connection->log, ngx_errno,
+                      "libxml2 error");
+        return;
+    }
+
+    if (n > NGX_MAX_ERROR_STR) {
+        n = NGX_MAX_ERROR_STR;
+    }
+
     while (--n && (buf[n] == CR || buf[n] == LF)) { /* void */ }
 
     ngx_log_error(NGX_LOG_ERR, ctx->request->connection->log, 0,
-                  "libxml2 error: \"%*s\"", n + 1, buf);
+                  "libxml2 error: \"%*s\"", (size_t) (n + 1), buf);
 }
 
 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-05a6c77b2b` → 本草稿移入 `cases/defect/auto-nginx-05a6c77b2b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
