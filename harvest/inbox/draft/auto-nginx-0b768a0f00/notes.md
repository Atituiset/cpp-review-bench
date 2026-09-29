# auto-nginx-0b768a0f00

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1561 (https://github.com/nginx/nginx/pull/1561)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 190；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -126,6 +126,7 @@ ngx_http_index_handler(ngx_http_request_t *r)
     name = NULL;
     /* suppress MSVC warning */
     path.data = NULL;
+    e.status = 0;
 
     index = ilcf->indices->elts;
     for (i = 0; i < ilcf->indices->nelts; i++) {
@@ -180,18 +181,28 @@ ngx_http_index_handler(ngx_http_request_t *r)
         } else {
             e.ip = index[i].values->elts;
             e.pos = name;
+            e.end = name + allocated;
 
             while (*(uintptr_t *) e.ip) {
                 code = *(ngx_http_script_code_pt *) e.ip;
                 code((ngx_http_script_engine_t *) &e);
             }
 
+            if (e.status) {
+                return NGX_HTTP_INTERNAL_SERVER_ERROR;
+            }
+
+            if (ngx_http_script_check_length(&e, 1) != NGX_OK) {
+                return NGX_ERROR;
+            }
+
             if (*name == '/') {
-                uri.len = len - 1;
+                uri.len = e.pos - name;
                 uri.data = name;
                 return ngx_http_internal_redirect(r, &uri, &r->args);
             }
 
+            len = e.pos - name + 1;
             path.len = e.pos - path.data;
 
             *e.pos = '\0';
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-0b768a0f00` → 本草稿移入 `cases/defect/auto-nginx-0b768a0f00/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
