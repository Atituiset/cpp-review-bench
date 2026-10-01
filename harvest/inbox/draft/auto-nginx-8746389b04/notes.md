# auto-nginx-8746389b04

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #966 (https://github.com/nginx/nginx/pull/966)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 133；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -130,8 +130,8 @@ ngx_int_t ngx_http_post_request(ngx_http_request_t *r,
     ngx_http_posted_request_t *pr);
 ngx_int_t ngx_http_set_virtual_server(ngx_http_request_t *r,
     ngx_str_t *host);
-ngx_int_t ngx_http_validate_host(ngx_str_t *host, ngx_pool_t *pool,
-    ngx_uint_t alloc);
+ngx_int_t ngx_http_validate_host(ngx_str_t *host, in_port_t *port,
+    ngx_pool_t *pool, ngx_uint_t alloc);
 void ngx_http_close_request(ngx_http_request_t *r, ngx_int_t rc);
 void ngx_http_finalize_request(ngx_http_request_t *r, ngx_int_t rc);
 void ngx_http_free_request(ngx_http_request_t *r, ngx_int_t rc);
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-8746389b04` → 本草稿移入 `cases/defect/auto-nginx-8746389b04/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
