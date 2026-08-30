# auto-nginx-a04ca9c5c0

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1637 (https://github.com/nginx/nginx/pull/1637)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 416；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -413,7 +413,11 @@ ngx_http_memcached_process_header(ngx_http_request_t *r)
         p = line.data + line.len;
 
         u->headers_in.content_length_n = ngx_atoof(start, p - start);
-        if (u->headers_in.content_length_n == NGX_ERROR) {
+
+        if (u->headers_in.content_length_n == NGX_ERROR
+            || u->headers_in.content_length_n
+               > NGX_MAX_OFF_T_VALUE - (off_t) NGX_HTTP_MEMCACHED_END)
+        {
             ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                           "memcached sent invalid length in response \"%V\" "
                           "for key \"%V\"",
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-a04ca9c5c0` → 本草稿移入 `cases/defect/auto-nginx-a04ca9c5c0/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
