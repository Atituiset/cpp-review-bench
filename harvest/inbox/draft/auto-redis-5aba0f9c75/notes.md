# auto-redis-5aba0f9c75

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | redis/redis |
| 源 PR | [#15803](https://github.com/redis/redis/pull/15803) |
| 许可证 | RSALv2 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-27 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 7 |
| 编译错误数（gcc syntax-only） | 8（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #15803 (https://github.com/redis/redis/pull/15803)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 5876；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 15803 Redis 8.8.3 :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3707,7 +3707,6 @@ int processInputBuffer(client *c) {
                 pcmd->reploff = c->io_read_reploff - sdslen(c->querybuf) + c->qb_pos;
 
             preprocessCommand(c, pcmd);
-            pcmd->flags |= PENDING_CMD_FLAG_PREPROCESSED;
             resetClientQbufState(c);
         }
 
@@ -5868,20 +5867,21 @@ pendingCommand *popPendingCommandFromTail(pendingCommandList *list) {
     return cmd;
 }
 
-/* Get cached key result for current pending command */
-getKeysResult *getClientCachedKeyResult(client *c) {
-    pendingCommand *pcmd = c->current_pending_cmd;
-    if (pcmd) {
-        /* Preprocess the command if needed */
-        if (!(pcmd->flags & PENDING_CMD_FLAG_PREPROCESSED)) {
-            preprocessCommand(c, pcmd);
-            pcmd->flags |= PENDING_CMD_FLAG_PREPROCESSED;
-        }
-
-        /* Return cached result if available */
-        if (pcmd->flags & PENDING_CMD_KEYS_RESULT_VALID)
-            return &c->current_pending_cmd->keys_result;
-    }
+/* Get the cached key result of 'pcmd', or NULL if it has none, in which case
+ * the caller is expected to extract the keys from the command arguments itself.
+ *
+ * 'pcmd' must be the pendingCommand that 'c->cmd' / 'c->argv' were populated
+ * from: the cached result records key positions within that command's argv, so
+ * handing over an unrelated pendingCommand (for instance the client's current
+ * pending command while a queued MULTI command is being executed) would return
+ * key positions that have nothing to do with the command being checked. */
+getKeysResult *getClientCachedKeyResult(pendingCommand *pcmd) {
+    if (!pcmd) return NULL;
+    serverAssert(pcmd->flags & PENDING_CMD_FLAG_PREPROCESSED);
+
+    /* Return cached result if available */
+    if (pcmd->flags & PENDING_CMD_KEYS_RESULT_VALID)
+        return &pcmd->keys_result;
     return NULL;
 }
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`preprocessCommand`
- 外部函数：`sdslen`
- 大写宏：`NULL`
- 大写宏：`PENDING_CMD_FLAG_PREPROCESSED`
- 大写宏：`PENDING_CMD_KEYS_RESULT_VALID`
- 外部类型：`Get`
- 外部类型：`Preprocess`
- 外部类型：`Return`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-redis-5aba0f9c75` → 本草稿移入 `cases/defect/auto-redis-5aba0f9c75/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
