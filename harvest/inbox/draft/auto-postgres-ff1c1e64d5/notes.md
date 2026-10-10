# auto-postgres-ff1c1e64d5

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#15a947f30726cadff695b7601329d90b6703e268](https://github.com/postgres/postgres/commit/15a947f30726cadff695b7601329d90b6703e268) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-10 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 35 |
| 编译错误数（gcc syntax-only） | 25（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #15a947f30726cadff695b7601329d90b6703e268 (https://github.com/postgres/postgres/commit/15a947f30726cadff695b7601329d90b6703e268)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 27；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 15a947f30726cadff695b7601329d90b6703e268 refactor: Move BackendWritebackContext initialization :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -24,17 +24,14 @@
 BufferDescPadded *BufferDescriptors;
 char	   *BufferBlocks;
 ConditionVariableMinimallyPadded *BufferIOCVArray;
-WritebackContext BackendWritebackContext;
 CkptSortItem *CkptBufferIds;
 
 static void BufferManagerShmemRequest(void *arg);
 static void BufferManagerShmemInit(void *arg);
-static void BufferManagerShmemAttach(void *arg);
 
 const ShmemCallbacks BufferManagerShmemCallbacks = {
 	.request_fn = BufferManagerShmemRequest,
 	.init_fn = BufferManagerShmemInit,
-	.attach_fn = BufferManagerShmemAttach,
 };
 
 /*
@@ -138,16 +135,4 @@ BufferManagerShmemInit(void *arg)
 		proclist_init(&buf->lock_waiters);
 		ConditionVariableInit(BufferDescriptorGetIOCV(buf));
 	}
-
-	/* Initialize per-backend file flush context */
-	WritebackContextInit(&BackendWritebackContext,
-						 &backend_flush_after);
-}
-
-static void
-BufferManagerShmemAttach(void *arg)
-{
-	/* Initialize per-backend file flush context */
-	WritebackContextInit(&BackendWritebackContext,
-						 &backend_flush_after);
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`pg_atomic_init_u64`
- 外部函数：`pgaio_wref_clear`
- 外部函数：`proclist_init`
- 大写宏：`BLCKSZ`
- 大写宏：`INVALID_PROC_NUMBER`
- 大写宏：`PG_CACHE_LINE_SIZE`
- 大写宏：`PG_IO_ALIGN_SIZE`
- 外部类型：`Align`
- 外部类型：`As`
- 外部类型：`BackendWritebackContext`
- 外部类型：`Blocks`
- 外部类型：`Buffer`
- 外部类型：`BufferBlocks`
- 外部类型：`BufferDesc`
- 外部类型：`BufferDescPadded`
- 外部类型：`BufferDescriptors`
- 外部类型：`BufferIOCVArray`
- 外部类型：`BufferIds`
- 外部类型：`BufferManagerShmemAttach`
- 外部类型：`BufferManagerShmemCallbacks`
- 外部类型：`BufferManagerShmemInit`
- 外部类型：`BufferManagerShmemRequest`
- 外部类型：`Checkpoint`
- 外部类型：`CkptBufferIds`
- 外部类型：`CkptSortItem`
- 外部类型：`Condition`
- 外部类型：`ConditionVariableMinimallyPadded`
- 外部类型：`Descriptors`
- 外部类型：`Initialize`
- 外部类型：`NBuffers`
- 外部类型：`ShmemCallbacks`
- 外部类型：`Size`
- 外部类型：`The`
- 外部类型：`Variables`
- 外部类型：`WritebackContext`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-ff1c1e64d5` → 本草稿移入 `cases/defect/auto-postgres-ff1c1e64d5/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
