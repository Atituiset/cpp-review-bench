# auto-redis-6756e2b341

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15673 (https://github.com/redis/redis/pull/15673)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 1958；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -1954,9 +1954,18 @@ int ACLCheckAllUserCommandPerm(user *u, struct redisCommand *cmd, robj **argv, i
     return relevant_error;
 }
 
-/* High level API for checking if a client can execute the queued up command */
-int ACLCheckAllPerm(client *c, int *idxptr) {
-    return ACLCheckAllUserCommandPerm(c->user, c->cmd, c->argv, c->argc, getClientCachedKeyResult(c), idxptr);
+/* High level API for checking if a client can execute the queued up command.
+ *
+ * 'pcmd' is the pendingCommand that c->cmd / c->argv were populated from, and
+ * is used to reuse its cached key extraction result. It may be NULL for clients
+ * that don't build pendingCommand structs (module and script fake clients), in
+ * which case the keys are extracted from c->argv. */
+int ACLCheckAllPerm(client *c, pendingCommand *pcmd, int *idxptr) {
+    /* The cached key result holds key positions within pcmd->argv, so it is only
+     * usable for the command we are actually about to check. */
+    serverAssert(!pcmd || (pcmd->cmd == c->cmd));
+    return ACLCheckAllUserCommandPerm(c->user, c->cmd, c->argv, c->argc,
+                getClientCachedKeyResult(pcmd), idxptr);
 }
 
 /* If 'new' can access all channels 'original' could then return NULL;
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-6756e2b341` → 本草稿移入 `cases/defect/auto-redis-6756e2b341/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
