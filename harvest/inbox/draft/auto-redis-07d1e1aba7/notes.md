# auto-redis-07d1e1aba7

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #14974 (https://github.com/redis/redis/pull/14974)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -147,6 +147,8 @@ int AuthAsyncCommand_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv,
     targs[1] = RedisModule_CreateStringFromString(NULL, argv[1]);
 
     if (pthread_create(&tid, NULL, HelloACL_ThreadMain, targs) != 0) {
+        RedisModule_FreeString(NULL, targs[1]);
+        RedisModule_Free(targs);
         RedisModule_AbortBlock(bc);
         return RedisModule_ReplyWithError(ctx, "-ERR Can't start thread");
     }
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-07d1e1aba7` → 本草稿移入 `cases/defect/auto-redis-07d1e1aba7/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
