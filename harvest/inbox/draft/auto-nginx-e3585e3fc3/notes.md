# auto-nginx-e3585e3fc3

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #750 (https://github.com/nginx/nginx/pull/750)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 2537；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1123,7 +1123,7 @@ ngx_http_upstream_cache_send(ngx_http_request_t *r, ngx_http_upstream_t *u)
         return NGX_ERROR;
     }
 
-    if (rc == NGX_AGAIN) {
+    if (rc == NGX_AGAIN || rc == NGX_HTTP_UPSTREAM_EARLY_HINTS) {
         rc = NGX_HTTP_UPSTREAM_INVALID_HEADER;
     }
 
@@ -2533,9 +2533,7 @@ ngx_http_upstream_process_header(ngx_http_request_t *r, ngx_http_upstream_t *u)
             continue;
         }
 
-        if (rc == NGX_OK
-            && u->headers_in.status_n == NGX_HTTP_EARLY_HINTS)
-        {
+        if (rc == NGX_HTTP_UPSTREAM_EARLY_HINTS) {
             rc = ngx_http_upstream_process_early_hints(r, u);
 
             if (rc == NGX_OK) {
@@ -2651,10 +2649,6 @@ ngx_http_upstream_process_early_hints(ngx_http_request_t *r,
         }
     }
 
-    if (u->reinit_request(r) != NGX_OK) {
-        return NGX_ERROR;
-    }
-
     ngx_http_clean_header(r);
 
     ngx_memzero(&u->headers_in, sizeof(ngx_http_upstream_headers_in_t));
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-e3585e3fc3` → 本草稿移入 `cases/defect/auto-nginx-e3585e3fc3/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
