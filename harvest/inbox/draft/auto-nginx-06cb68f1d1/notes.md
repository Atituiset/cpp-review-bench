# auto-nginx-06cb68f1d1

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #772 (https://github.com/nginx/nginx/pull/772)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 1（原始 PR diff 行 1037；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1003,6 +1003,7 @@ ngx_http_v3_process_request_header(ngx_http_request_t *r)
 {
     ssize_t                  n;
     ngx_buf_t               *b;
+    ngx_str_t                host;
     ngx_connection_t        *c;
     ngx_http_v3_session_t   *h3c;
     ngx_http_v3_srv_conf_t  *h3scf;
@@ -1034,11 +1035,13 @@ ngx_http_v3_process_request_header(ngx_http_request_t *r)
         goto failed;
     }
 
-    if (r->headers_in.host) {
-        if (r->headers_in.host->value.len != r->headers_in.server.len
-            || ngx_memcmp(r->headers_in.host->value.data,
-                          r->headers_in.server.data,
-                          r->headers_in.server.len)
+    if (r->headers_in.host && r->host_end) {
+
+        host.len = r->host_end - r->host_start;
+        host.data = r->host_start;
+
+        if (r->headers_in.host->value.len != host.len
+            || ngx_memcmp(r->headers_in.host->value.data, host.data, host.len)
                != 0)
         {
             ngx_log_error(NGX_LOG_INFO, c->log, 0,
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-06cb68f1d1` → 本草稿移入 `cases/defect/auto-nginx-06cb68f1d1/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
