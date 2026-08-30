# auto-nginx-75f715560c

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #515 (https://github.com/nginx/nginx/pull/515)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 2441；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -2327,6 +2327,7 @@ ngx_http_subrequest(ngx_http_request_t *r,
     ngx_connection_t              *c;
     ngx_http_request_t            *sr;
     ngx_http_core_srv_conf_t      *cscf;
+    ngx_http_posted_request_t     *posted;
     ngx_http_postponed_request_t  *pr, *p;
 
     if (r->subrequests == 0) {
@@ -2380,6 +2381,11 @@ ngx_http_subrequest(ngx_http_request_t *r,
         return NGX_ERROR;
     }
 
+    posted = ngx_palloc(r->pool, sizeof(ngx_http_posted_request_t));
+    if (posted == NULL) {
+        return NGX_ERROR;
+    }
+
     cscf = ngx_http_get_module_srv_conf(r, ngx_http_core_module);
     sr->main_conf = cscf->ctx->main_conf;
     sr->srv_conf = cscf->ctx->srv_conf;
@@ -2438,10 +2444,6 @@ ngx_http_subrequest(ngx_http_request_t *r,
     }
 
     if (!sr->background) {
-        if (c->data == r && r->postponed == NULL) {
-            c->data = sr;
-        }
-
         pr = ngx_palloc(r->pool, sizeof(ngx_http_postponed_request_t));
         if (pr == NULL) {
             return NGX_ERROR;
@@ -2451,6 +2453,10 @@ ngx_http_subrequest(ngx_http_request_t *r,
         pr->out = NULL;
         pr->next = NULL;
 
+        if (c->data == r && r->postponed == NULL) {
+            c->data = sr;
+        }
+
         if (r->postponed) {
             for (p = r->postponed; p->next; p = p->next) { /* void */ }
             p->next = pr;
@@ -2498,7 +2504,7 @@ ngx_http_subrequest(ngx_http_request_t *r,
         ngx_http_update_location_config(sr);
     }
 
-    return ngx_http_post_request(sr, NULL);
+    return ngx_http_post_request(sr, posted);
 }
 
 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-75f715560c` → 本草稿移入 `cases/defect/auto-nginx-75f715560c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
