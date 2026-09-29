# auto-nginx-39734dcf51

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1556 (https://github.com/nginx/nginx/pull/1556)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -50,9 +50,18 @@ typedef struct {
 } ngx_http_perl_var_t;
 
 
+typedef struct {
+    ngx_http_request_t       *request;
+    SV                       *sv;
+} ngx_http_perl_cleanup_t;
+
+
 extern ngx_module_t  ngx_http_perl_module;
 
 
+extern ngx_http_perl_ctx_t  *ngx_http_perl_active_context;
+
+
 /*
  * workaround for "unused variable `Perl___notused'" warning
  * when building with perl 5.6.1
@@ -68,6 +77,7 @@ extern void boot_DynaLoader(pTHX_ CV* cv);
 
 void ngx_http_perl_handle_request(ngx_http_request_t *r);
 void ngx_http_perl_sleep_handler(ngx_http_request_t *r);
+void ngx_http_perl_refcount_cleanup(void *data);
 
 
 #endif /* _NGX_HTTP_PERL_MODULE_H_INCLUDED_ */
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-39734dcf51` → 本草稿移入 `cases/defect/auto-nginx-39734dcf51/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
