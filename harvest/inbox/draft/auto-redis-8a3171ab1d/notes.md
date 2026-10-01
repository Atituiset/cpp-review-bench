# auto-redis-8a3171ab1d

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15356 (https://github.com/redis/redis/pull/15356)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 2468；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -2464,8 +2464,11 @@ sds ACLLoadFromFile(const char *filename) {
         listRewind(server.clients,&li);
         while ((ln = listNext(&li)) != NULL) {
             client *c = listNodeValue(ln);
-            /* a MASTER client can do everything (and user = NULL) so we can skip it */
-            if (c->flags & CLIENT_MASTER)
+            /* Clients with no associated user (user = NULL) have nothing to
+             * re-resolve and must be skipped before dereferencing c->user.
+             * This covers MASTER clients as well as internal connections
+             * (CLIENT_INTERNAL), both of which run without a user. */
+            if (c->user == NULL)
                 continue;
             user *original = c->user;
             list *channels = NULL;
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-8a3171ab1d` → 本草稿移入 `cases/defect/auto-redis-8a3171ab1d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
