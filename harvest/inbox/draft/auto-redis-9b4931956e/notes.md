# auto-redis-9b4931956e

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
| 外部依赖数（dep_count） | 9 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #16303 (https://github.com/redis/redis/pull/16303)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 4985；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

PR 16303 Fix crash when slot migration destination has a disk error :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -4982,7 +4982,9 @@ int processCommand(client *c) {
     if (deny_write_type != DISK_ERROR_TYPE_NONE &&
         (is_write_command || c->cmd->proc == pingCommand))
     {
-        if (obey_client) {
+        /* The ASM import client is a master client, but this node is not a
+         * replica. Reject the command, the import task fails instead. */
+        if (obey_client && !(c->flags & CLIENT_ASM_IMPORTING)) {
             if (!server.repl_ignore_disk_write_error && c->cmd->proc != pingCommand) {
                 serverPanic("Replica was unable to write command to disk.");
             } else {
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`addReply`
- 外部函数：`addReplyBulk`
- 外部函数：`addReplyBulkCBuffer`
- 外部函数：`addReplyErrorArity`
- 外部函数：`serverPanic`
- 大写宏：`CLIENT_PUBSUB`
- 大写宏：`DISK_ERROR_TYPE_NONE`
- 外部类型：`Replica`
- 外部类型：`The`

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

1. 完成上面检查清单后评论 `/case accept auto-redis-9b4931956e` → 本草稿移入 `cases/defect/auto-redis-9b4931956e/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
