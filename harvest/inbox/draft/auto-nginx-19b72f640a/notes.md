# auto-nginx-19b72f640a

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1601 (https://github.com/nginx/nginx/pull/1601)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 1881；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1867,20 +1867,28 @@ ngx_http_script_complex_value_code(ngx_http_script_engine_t *e)
 
     e->pos = e->buf.data;
     e->end = e->buf.data + len;
+
+    e->sp->len = e->buf.len;
+    e->sp->data = e->buf.data;
+    e->sp++;
 }
 
 
 void
 ngx_http_script_complex_value_end_code(ngx_http_script_engine_t *e)
 {
+    ngx_http_variable_value_t  *val;
+
+    val = e->sp - 1;
+
     e->ip += sizeof(ngx_http_script_complex_value_end_code_t);
 
     ngx_log_debug0(NGX_LOG_DEBUG_HTTP, e->request->connection->log, 0,
                    "http script complex value end");
 
-    e->sp->len = e->pos - e->buf.data;
-    e->sp->data = e->buf.data;
-    e->sp++;
+    if (val->data == e->buf.data) {
+        val->len = e->pos - e->buf.data;
+    }
 }
 
 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-19b72f640a` → 本草稿移入 `cases/defect/auto-nginx-19b72f640a/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
