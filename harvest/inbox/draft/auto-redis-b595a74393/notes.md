# auto-redis-b595a74393

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15659 (https://github.com/redis/redis/pull/15659)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 6378；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -6375,7 +6375,7 @@ RedisModuleCallReply *RM_Call(RedisModuleCtx *ctx, const char *cmdname, const ch
             reply = callReplyCreateError(err, ctx);
         goto cleanup;
     }
-    if (!commandCheckArity(c, error_as_call_replies? &err : NULL)) {
+    if (!commandCheckArity(c->cmd, c->argc, error_as_call_replies? &err : NULL)) {
         errno = EINVAL;
         if (error_as_call_replies)
             reply = callReplyCreateError(err, ctx);
@@ -9702,6 +9702,7 @@ RedisModuleUser *RM_GetModuleUserFromUserName(RedisModuleString *name) {
  * REDISMODULE_ERR is returned and errno is set to the following values:
  *
  * * ENOENT: Specified command does not exist.
+ * * EINVAL: Invalid number of arguments for the specified command.
  * * EACCES: Command cannot be executed, according to ACL rules
  */
 int RM_ACLCheckCommandPermissions(RedisModuleUser *user, RedisModuleString **argv, int argc) {
@@ -9714,6 +9715,11 @@ int RM_ACLCheckCommandPermissions(RedisModuleUser *user, RedisModuleString **arg
         return REDISMODULE_ERR;
     }
 
+    if (!commandCheckArity(cmd, argc, NULL)) {
+        errno = EINVAL;
+        return REDISMODULE_ERR;
+    }
+
     if (ACLCheckAllUserCommandPerm(user->user, cmd, argv, argc, &keyidxptr) != ACL_OK) {
         errno = EACCES;
         return REDISMODULE_ERR;
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-b595a74393` → 本草稿移入 `cases/defect/auto-redis-b595a74393/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
