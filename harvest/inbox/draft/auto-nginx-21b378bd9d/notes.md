# auto-nginx-21b378bd9d

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #966 (https://github.com/nginx/nginx/pull/966)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 468；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -383,21 +383,18 @@ ngx_http_parse_request_line(ngx_http_request_t *r, ngx_buf_t *b)
 
         case sw_host_end:
 
+            if (ch == ':') {
+                state = sw_port;
+                break;
+            }
+
             r->host_end = p;
 
             if (r->method == NGX_HTTP_CONNECT) {
-                if (ch == ':') {
-                    state = sw_port;
-                    break;
-                }
-
                 return NGX_HTTP_PARSE_INVALID_REQUEST;
             }
 
             switch (ch) {
-            case ':':
-                state = sw_port;
-                break;
             case '/':
                 r->uri_start = p;
                 state = sw_after_slash_in_uri;
@@ -465,14 +462,11 @@ ngx_http_parse_request_line(ngx_http_request_t *r, ngx_buf_t *b)
 
         case sw_port:
             if (ch >= '0' && ch <= '9') {
-                if (r->port >= 6553 && (r->port > 6553 || (ch - '0') > 5)) {
-                    return NGX_HTTP_PARSE_INVALID_REQUEST;
-                }
-
-                r->port = r->port * 10 + (ch - '0');
                 break;
             }
 
+            r->host_end = p;
+
             if (r->method == NGX_HTTP_CONNECT) {
                 if (ch == ' ') {
                     state = sw_http_09;
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-21b378bd9d` → 本草稿移入 `cases/defect/auto-nginx-21b378bd9d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
