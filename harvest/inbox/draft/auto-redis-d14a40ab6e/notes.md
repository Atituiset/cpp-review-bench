# auto-redis-d14a40ab6e

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | redis/redis |
| 源 PR | [#16303](https://github.com/redis/redis/pull/16303) |
| 许可证 | RSALv2 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-23 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 32 |
| 编译错误数（gcc syntax-only） | 21（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #16303 (https://github.com/redis/redis/pull/16303)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 91；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 16303 Fix crash when slot migration destination has a disk error :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -88,7 +88,7 @@ typedef struct asmTask {
     client *main_channel_client;            /* Client for the main channel on the source side */
     client *rdb_channel_client;             /* Client for the RDB channel on the source side */
     long long retry_count;                  /* Number of retries for this task */
-    int retry_postponed;                    /* Logged that the retry waits for good replicas */
+    int retry_postponed;                    /* Logged that the retry waits for writes to be accepted */
     mstime_t create_time;                   /* Task creation time */
     mstime_t start_time;                    /* Task start time */
     mstime_t end_time;                      /* Task end time */
@@ -2910,12 +2910,18 @@ void asmBeforeSleep(void) {
 
 /* Restart a failed import task. */
 static void asmImportRetry(asmTask *task) {
-    /* If min-replicas-to-write is not satisfied, this node rejects the
-     * commands of the import stream and the task fails again. Do not
-     * retry until there are enough good replicas, log it once. */
-    if (!checkGoodReplicasStatus()) {
+    /* If this node denies writes (min-replicas-to-write is not satisfied or
+     * there is a disk error), it rejects the import stream and the task
+     * fails again. Do not retry until writes are accepted, log it once. */
+    const char *reason = NULL;
+    if (!checkGoodReplicasStatus())
+        reason = "not enough good replicas";
+    else if (writeCommandsDeniedByDiskError() != DISK_ERROR_TYPE_NONE)
+        reason = "writes are denied by a disk error";
+
+    if (reason) {
         if (!task->retry_postponed)
-            serverLog(LL_NOTICE, "Import task %s is not retried, not enough good replicas", task->id);
+            serverLog(LL_NOTICE, "Import task %s is not retried, %s", task->id, reason);
         task->retry_postponed = 1;
         return;
     }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`checkGoodReplicasStatus`
- 外部函数：`serverLog`
- 外部函数：`state`
- 大写宏：`ASM_IMPORT`
- 大写宏：`ASM_MIGRATE`
- 大写宏：`CLUSTER_NAMELEN`
- 大写宏：`EOF`
- 大写宏：`LL_NOTICE`
- 大写宏：`RDB`
- 大写宏：`STREAM`
- 外部类型：`Already`
- 外部类型：`Buffer`
- 外部类型：`Client`
- 外部类型：`Current`
- 外部类型：`Destination`
- 外部类型：`Do`
- 外部类型：`Either`
- 外部类型：`Error`
- 外部类型：`If`
- 外部类型：`Import`
- 外部类型：`List`
- 外部类型：`Logged`
- 外部类型：`Main`
- 外部类型：`Module`
- 外部类型：`Number`
- 外部类型：`Restart`
- 外部类型：`Snapshot`
- 外部类型：`Source`
- 外部类型：`State`
- 外部类型：`Task`
- 外部类型：`The`
- 外部类型：`Total`
- 外部类型：`mstime_t`

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

1. 完成上面检查清单后评论 `/case accept auto-redis-d14a40ab6e` → 本草稿移入 `cases/defect/auto-redis-d14a40ab6e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
