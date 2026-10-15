# auto-nginx-bccd34b03e

## 来源（采集溯源）
- 来源仓: nginx/nginx
- 源 PR: #1664 (https://github.com/nginx/nginx/pull/1664)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 1114；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -253,6 +253,7 @@ ngx_event_pipe_read_upstream(ngx_event_pipe_t *p)
                 chain->next = NULL;
 
             } else if (!p->cacheable
+                       && !p->downstream_error
                        && p->downstream->data == p->output_ctx
                        && p->downstream->write->ready
                        && !p->downstream->write->delayed)
@@ -1111,11 +1112,7 @@ ngx_event_pipe_drain_chains(ngx_event_pipe_t *p)
     ngx_chain_t  *cl, *tl;
 
     for ( ;; ) {
-        if (p->busy) {
-            cl = p->busy;
-            p->busy = NULL;
-
-        } else if (p->out) {
+        if (p->out) {
             cl = p->out;
             p->out = NULL;
 
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-nginx-bccd34b03e` → 本草稿移入 `cases/defect/auto-nginx-bccd34b03e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
