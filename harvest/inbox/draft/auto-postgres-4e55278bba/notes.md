# auto-postgres-4e55278bba

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#45047c51aab3b02035217088ef35220b2be905d7](https://github.com/postgres/postgres/commit/45047c51aab3b02035217088ef35220b2be905d7) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-18 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 18 |
| 编译错误数（gcc syntax-only） | 12（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #45047c51aab3b02035217088ef35220b2be905d7 (https://github.com/postgres/postgres/commit/45047c51aab3b02035217088ef35220b2be905d7)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 425；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 45047c51aab3b02035217088ef35220b2be905d7 Fix temporary relation I/O statistics in single-user mode :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -413,16 +413,16 @@ pgstat_tracks_io_object(BackendType bktype, IOObject io_object,
 		return false;
 
 	/*
-	 * In core Postgres, only regular backends and WAL Sender processes
-	 * executing queries will use local buffers and operate on temporary
-	 * relations. Parallel workers will not use local buffers (see
-	 * InitLocalBuffers()); however, extensions leveraging background workers
-	 * have no such limitation, so track IO on IOOBJECT_TEMP_RELATION for
-	 * BackendType B_BG_WORKER.
+	 * In core Postgres, only regular backends, WAL Sender processes executing
+	 * queries and standalone backends (single-user mode) will use local
+	 * buffers and operate on temporary relations. Parallel workers will not
+	 * use local buffers (see InitLocalBuffers()); however, extensions
+	 * leveraging background workers have no such limitation, so track IO on
+	 * IOOBJECT_TEMP_RELATION for BackendType B_BG_WORKER.
 	 */
 	no_temp_rel = bktype == B_AUTOVAC_LAUNCHER || bktype == B_BG_WRITER ||
 		bktype == B_CHECKPOINTER || bktype == B_AUTOVAC_WORKER ||
-		bktype == B_STANDALONE_BACKEND || bktype == B_STARTUP ||
+		bktype == B_STARTUP ||
 		bktype == B_WAL_SUMMARIZER || bktype == B_WAL_WRITER ||
 		bktype == B_WAL_RECEIVER;
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`buffers`
- 大写宏：`B_AUTOVAC_LAUNCHER`
- 大写宏：`B_AUTOVAC_WORKER`
- 大写宏：`B_BG_WORKER`
- 大写宏：`B_BG_WRITER`
- 大写宏：`B_CHECKPOINTER`
- 大写宏：`B_STANDALONE_BACKEND`
- 大写宏：`B_STARTUP`
- 大写宏：`B_WAL_RECEIVER`
- 大写宏：`B_WAL_SUMMARIZER`
- 大写宏：`B_WAL_WRITER`
- 大写宏：`IOOBJECT_TEMP_RELATION`
- 大写宏：`WAL`
- 外部类型：`BackendType`
- 外部类型：`In`
- 外部类型：`Parallel`
- 外部类型：`Postgres`
- 外部类型：`Sender`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-4e55278bba` → 本草稿移入 `cases/defect/auto-postgres-4e55278bba/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
