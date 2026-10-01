# auto-nginx-9172cc5a37

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1395 (https://github.com/nginx/nginx/pull/1395)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 1147；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1037,6 +1037,8 @@ ngx_http_script_start_args_code(ngx_http_script_engine_t *e)
 void
 ngx_http_script_regex_start_code(ngx_http_script_engine_t *e)
 {
+    int                           *cap;
+    u_char                        *p;
     size_t                         len;
     ngx_int_t                      rc;
     ngx_uint_t                     n;
@@ -1143,15 +1145,19 @@ ngx_http_script_regex_start_code(ngx_http_script_engine_t *e)
     if (code->lengths == NULL) {
         e->buf.len = code->size;
 
-        if (code->uri) {
-            if (r->ncaptures && (r->quoted_uri || r->plus_in_uri)) {
-                e->buf.len += 2 * ngx_escape_uri(NULL, r->uri.data, r->uri.len,
-                                                 NGX_ESCAPE_ARGS);
-            }
-        }
+        cap = r->captures;
+        p = r->captures_data;
 
         for (n = 2; n < r->ncaptures; n += 2) {
-            e->buf.len += r->captures[n + 1] - r->captures[n];
+            e->buf.len += cap[n + 1] - cap[n];
+
+            if (code->uri) {
+                if (r->quoted_uri || r->plus_in_uri) {
+                    e->buf.len += 2 * ngx_escape_uri(NULL, &p[cap[n]],
+                                                     cap[n + 1] - cap[n],
+                                                     NGX_ESCAPE_ARGS);
+                }
+            }
         }
 
     } else {
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-9172cc5a37` → 本草稿移入 `cases/defect/auto-nginx-9172cc5a37/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
