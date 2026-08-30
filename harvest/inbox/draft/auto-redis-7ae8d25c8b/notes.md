# auto-redis-7ae8d25c8b

## 来源（采集溯源）
- 来源仓: redis/redis
- 源 PR: #15432 (https://github.com/redis/redis/pull/15432)
- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 真实修复前后 diff（PR 改了什么）

```diff
@@ -12815,22 +12815,11 @@ static uint64_t moduleEventVersions[] = {
  *
  * * RedisModuleEvent_ForkChild
  *
- *     Called when a fork child (AOFRW, RDBSAVE, module fork...) is born or dies,
- *     and around the fork itself so a multi-threaded module can bring its
- *     background threads to a safe point before `fork()`. This matters because a
- *     thread holding a lock (for example the allocator lock) at `fork()` time
- *     would deadlock the child the first time it tries to take that lock.
- *     `REDISMODULE_SUBEVENT_FORK_CHILD_PRE` is fired synchronously on the main
- *     thread right before `fork()` (returning from the handler tells Redis the
- *     module is ready); the module then resumes on
- *     `REDISMODULE_SUBEVENT_FORK_CHILD_BORN` if the fork happened or on
- *     `REDISMODULE_SUBEVENT_FORK_CHILD_CANCELLED` if it did not.
+ *     Called when a fork child (AOFRW, RDBSAVE, module fork...) is born/dies
  *     The following sub events are available:
  *
  *     * `REDISMODULE_SUBEVENT_FORK_CHILD_BORN`
  *     * `REDISMODULE_SUBEVENT_FORK_CHILD_DIED`
- *     * `REDISMODULE_SUBEVENT_FORK_CHILD_PRE`
- *     * `REDISMODULE_SUBEVENT_FORK_CHILD_CANCELLED`
  *
  * * RedisModuleEvent_EventLoop
  *
```

## 接受后流程（accept → case）
1. 评论 `/case accept auto-redis-7ae8d25c8b` → 本草稿移入 `cases/defect/auto-redis-7ae8d25c8b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
