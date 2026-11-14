# auto-redis-a0c49f020a

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | redis/redis |
| 源 PR | [#16042](https://github.com/redis/redis/pull/16042) |
| 许可证 | RSALv2 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-14 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 32 |
| 编译错误数（gcc syntax-only） | 18（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #16042 (https://github.com/redis/redis/pull/16042)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 898；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 16042 Fix IO-thread crash in module getkeys/getchannels callbacks :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -895,7 +895,9 @@ void modulePostExecutionUnitOperations(void) {
 /* Free the context after the user function was called. */
 void moduleFreeContext(RedisModuleCtx *ctx) {
     /* See comment in moduleCreateContext */
-    if (!(ctx->flags & (REDISMODULE_CTX_THREAD_SAFE|REDISMODULE_CTX_COMMAND))) {
+    if (!(ctx->flags & (REDISMODULE_CTX_THREAD_SAFE|REDISMODULE_CTX_COMMAND|
+                        REDISMODULE_CTX_KEYS_POS_REQUEST|REDISMODULE_CTX_CHANNELS_POS_REQUEST)))
+    {
         exitExecutionUnit();
         postExecutionUnitOperationsEx(moduleTakeUnaccountedBackgroundDuration(ctx));
     }
@@ -991,8 +993,14 @@ void moduleCreateContext(RedisModuleCtx *out_ctx, RedisModule *module, int ctx_f
      *    In the former, the counter has been already incremented from within
      *    call() and in the latter we don't care about execution_nesting
      * 2. If we are running in a thread (execution_nesting will be dealt with
-     *    when locking/unlocking the GIL) */
-    if (!(ctx_flags & (REDISMODULE_CTX_THREAD_SAFE|REDISMODULE_CTX_COMMAND))) {
+     *    when locking/unlocking the GIL)
+     * 3. Key / channel position requests (getkeys-api / getchannels-api).
+     *    The callback only reports argument positions, and it can be invoked
+     *    from an IO thread (preprocessCommand() -> extractKeysAndSlot()), so it
+     *    must not touch the global execution_nesting / propagation state. */
+    if (!(ctx_flags & (REDISMODULE_CTX_THREAD_SAFE|REDISMODULE_CTX_COMMAND|
+                       REDISMODULE_CTX_KEYS_POS_REQUEST|REDISMODULE_CTX_CHANNELS_POS_REQUEST)))
+    {
         enterExecutionUnit(1, 0);
     }
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`enterExecutionUnit`
- 外部函数：`exitExecutionUnit`
- 外部函数：`postExecutionUnitOperationsEx`
- 外部函数：`thread`
- 外部函数：`void`
- 大写宏：`AOF`
- 大写宏：`API`
- 大写宏：`GIL`
- 大写宏：`NULL`
- 外部类型：`Already`
- 外部类型：`Called`
- 外部类型：`Context`
- 外部类型：`Current`
- 外部类型：`Database`
- 外部类型：`Fake`
- 外部类型：`Free`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`It`
- 外部类型：`Module`
- 外部类型：`Pointer`
- 外部类型：`RM_Unblock`
- 外部类型：`RedisModule`
- 外部类型：`RedisModuleAuthCallback`
- 外部类型：`RedisModuleBlockedClient`
- 外部类型：`RedisModuleCmdFunc`
- 外部类型：`RedisModuleCtx`
- 外部类型：`RedisModuleDisconnectFunc`
- 外部类型：`Reply`
- 外部类型：`See`
- 外部类型：`Set`
- 外部类型：`Slice`
- 外部类型：`Timer`
- 外部类型：`Used`
- 外部类型：`uint64_t`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。
- **依赖重（dep_count≥10）**：可考虑只做 PR/diff 形态评审，不做独立 case。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-redis-a0c49f020a` → 本草稿移入 `cases/defect/auto-redis-a0c49f020a/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
