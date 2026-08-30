# auto-redis-13895b7073

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #14690 (https://github.com/redis/redis/pull/14690)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -108,5 +108,12 @@ int RedisModule_OnLoad(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
     /* Trying to create a sub-subcommand fails */
     RedisModule_Assert(RedisModule_CreateSubcommand(subcmd,"get",NULL,"",0,0,0) == REDISMODULE_ERR);
 
+    /* Internal container command for testing */
+    if (RedisModule_CreateCommand(ctx,"subcommands.internal_container",NULL,"internal",0,0,0) == REDISMODULE_ERR)
+        return REDISMODULE_ERR;
+    RedisModuleCommand *internal_parent = RedisModule_GetCommand(ctx,"subcommands.internal_container");
+    if (RedisModule_CreateSubcommand(internal_parent,"test",cmd_set,"internal",0,0,0) == REDISMODULE_ERR)
+        return REDISMODULE_ERR;
+
     return REDISMODULE_OK;
 }
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-13895b7073` → 本草稿移入 `cases/defect/auto-redis-13895b7073/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
