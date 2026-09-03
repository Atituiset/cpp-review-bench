# auto-nginx-ed18c385df

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #646 (https://github.com/nginx/nginx/pull/646)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 48；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -45,8 +45,6 @@ static ssize_t ngx_ssl_sendfile(ngx_connection_t *c, ngx_buf_t *file,
     size_t size);
 static void ngx_ssl_read_handler(ngx_event_t *rev);
 static void ngx_ssl_shutdown_handler(ngx_event_t *ev);
-static void ngx_ssl_connection_error(ngx_connection_t *c, int sslerr,
-    ngx_err_t err, char *text);
 static void ngx_ssl_clear_error(ngx_log_t *log);
 
 static ngx_int_t ngx_ssl_session_id_context(ngx_ssl_t *ssl,
@@ -3301,7 +3299,7 @@ ngx_ssl_shutdown_handler(ngx_event_t *ev)
 }
 
 
-static void
+void
 ngx_ssl_connection_error(ngx_connection_t *c, int sslerr, ngx_err_t err,
     char *text)
 {
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-ed18c385df` → 本草稿移入 `cases/defect/auto-nginx-ed18c385df/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
