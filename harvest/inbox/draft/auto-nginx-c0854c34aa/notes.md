# auto-nginx-c0854c34aa

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1556 (https://github.com/nginx/nginx/pull/1556)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -148,6 +148,9 @@ static ngx_http_ssi_command_t  ngx_http_perl_ssi_command = {
 #endif
 
 
+ngx_http_perl_ctx_t     *ngx_http_perl_active_context;
+
+
 static ngx_str_t         ngx_null_name = ngx_null_string;
 static HV               *nginx_stash;
 
@@ -308,6 +311,29 @@ ngx_http_perl_sleep_handler(ngx_http_request_t *r)
 }
 
 
+void
+ngx_http_perl_refcount_cleanup(void *data)
+{
+    ngx_http_perl_cleanup_t  *clnp = data;
+
+    ngx_http_request_t         *r;
+    ngx_http_perl_main_conf_t  *pmcf;
+
+    r = clnp->request;
+    pmcf = ngx_http_get_module_main_conf(r, ngx_http_perl_module);
+
+    {
+
+    dTHXa(pmcf->perl);
+    PERL_SET_CONTEXT(pmcf->perl);
+    PERL_SET_INTERP(pmcf->perl);
+
+    SvREFCNT_dec(clnp->sv);
+
+    }
+}
+
+
 static ngx_int_t
 ngx_http_perl_variable(ngx_http_request_t *r, ngx_http_variable_value_t *v,
     uintptr_t data)
@@ -723,6 +749,8 @@ ngx_http_perl_call_handler(pTHX_ ngx_http_request_t *r,
 
     PUSHMARK(sp);
 
+    ngx_http_perl_active_context = ctx;
+
     sv = sv_2mortal(sv_bless(newRV_noinc(newSViv(PTR2IV(ctx))), nginx));
     XPUSHs(sv);
 
@@ -767,6 +795,8 @@ ngx_http_perl_call_handler(pTHX_ ngx_http_request_t *r,
     FREETMPS;
     LEAVE;
 
+    ngx_http_perl_active_context = NULL;
+
     if (ctx->error) {
 
         ngx_log_debug1(NGX_LOG_DEBUG_HTTP, c->log, 0,
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-c0854c34aa` → 本草稿移入 `cases/defect/auto-nginx-c0854c34aa/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
