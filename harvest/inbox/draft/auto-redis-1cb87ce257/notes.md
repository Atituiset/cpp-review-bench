# auto-redis-1cb87ce257

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15673 (https://github.com/redis/redis/pull/15673)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 2（原始 PR diff 行 197；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -190,11 +190,13 @@ void execCommand(client *c) {
         c->cmd = c->realcmd = c->mstate.commands[j]->cmd;
 
         /* ACL permissions are also checked at the time of execution in case
-         * they were changed after the commands were queued. */
+         * they were changed after the commands were queued. Note we pass the
+         * queued command itself, so that the key permissions are checked against
+         * its keys and not against the EXEC command we are currently running. */
         int acl_errpos;
         int acl_retval = ACL_OK;
         if (!skip_acl_check) {
-            acl_retval = ACLCheckAllPerm(c,&acl_errpos);
+            acl_retval = ACLCheckAllPerm(c,c->mstate.commands[j],&acl_errpos);
         }
         if (acl_retval != ACL_OK) {
             char *reason;
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-1cb87ce257` → 本草稿移入 `cases/defect/auto-redis-1cb87ce257/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
