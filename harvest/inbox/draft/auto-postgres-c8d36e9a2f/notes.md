# auto-postgres-c8d36e9a2f

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#cb1eef81c9bc8d1a2adc1e3a63669e315b6232bd](https://github.com/postgres/postgres/commit/cb1eef81c9bc8d1a2adc1e3a63669e315b6232bd) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-08 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 13 |
| 编译错误数（gcc syntax-only） | 7（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #cb1eef81c9bc8d1a2adc1e3a63669e315b6232bd (https://github.com/postgres/postgres/commit/cb1eef81c9bc8d1a2adc1e3a63669e315b6232bd)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT cb1eef81c9bc8d1a2adc1e3a63669e315b6232bd Fix WAL segment fd leaks with the WAL reading facility (xlogreader.c/h) :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -56,9 +56,12 @@ static bool ValidXLogRecord(XLogReaderState *state, XLogRecord *record,
 static void ResetDecoder(XLogReaderState *state);
 static void WALOpenSegmentInit(WALOpenSegment *seg, WALSegmentContext *segcxt,
 							   int segsize, const char *waldir);
-#if defined(USE_ZSTD) && !defined(FRONTEND)
+#ifndef FRONTEND
+static void xlogreader_memory_context_reset_cb(void *arg);
+#ifdef USE_ZSTD
 static void XLogReaderFreeZstdContext(void *arg);
 #endif
+#endif
 
 /* size of the buffer allocated for error message. */
 #define MAX_ERRORMSG_LEN 1000
@@ -166,6 +169,12 @@ XLogReaderAllocate(int wal_segment_size, const char *waldir,
 void
 XLogReaderFree(XLogReaderState *state)
 {
+#ifndef FRONTEND
+	if (state->reset_cb_registered)
+		MemoryContextUnregisterResetCallback(GetMemoryChunkContext(state),
+											 &state->reset_cb);
+#endif
+
 	if (state->seg.ws_file != -1)
 		state->routine.segment_close(state);
 
@@ -189,7 +198,43 @@ XLogReaderFree(XLogReaderState *state)
 	pfree(state);
 }
 
-#if defined(USE_ZSTD) && !defined(FRONTEND)
+#ifndef FRONTEND
+
+/*
+ * Memory context reset callback for an XLogReader.
+ */
+static void
+xlogreader_memory_context_reset_cb(void *arg)
+{
+	XLogReaderState *state = (XLogReaderState *) arg;
+
+	/* Close the WAL segment file that was left open */
+	if (state->seg.ws_file >= 0)
+		state->routine.segment_close(state);
+}
+
+/*
+ * Register a memory reset callback, closing a segment, if necessary.
+ *
+ * This is useful when opening a WAL segment file with BasicOpenFile(), which
+ * doesn't have the automatic file descriptor cleanup that OpenTransientFile()
+ * provides, to guarantee that the segment is closed before XLogReaderFree() is
+ * reached (on ERRORs, for example).
+ */
+void
+XLogReaderRegisterResetCallback(XLogReaderState *state)
+{
+	if (state->reset_cb_registered)
+		return;
+
+	state->reset_cb.func = xlogreader_memory_context_reset_cb;
+	state->reset_cb.arg = state;
+	MemoryContextRegisterResetCallback(GetMemoryChunkContext(state),
+									   &state->reset_cb);
+	state->reset_cb_registered = true;
+}
+
+#ifdef USE_ZSTD
 /* Release the zstd decomoression context. */
 static void
 XLogReaderFreeZstdContext(void *arg)
@@ -198,6 +243,8 @@ XLogReaderFreeZstdContext(void *arg)
 }
 #endif
 
+#endif							/* FRONTEND */
+
 /*
  * Allocate readRecordBuf to fit a record of at least the given length.
  *
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`pfree`
- 外部函数：`segment_close`
- 外部函数：`snprintf`
- 大写宏：`FRONTEND`
- 大写宏：`MAXPGPATH`
- 大写宏：`NULL`
- 大写宏：`USE_ZSTD`
- 外部类型：`Allocate`
- 外部类型：`Clear`
- 外部类型：`DecodedXLogRecord`
- 外部类型：`Release`
- 外部类型：`Reset`
- 外部类型：`WALOpenSegment`
- 外部类型：`WALSegmentContext`
- 外部类型：`XLogReaderState`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-c8d36e9a2f` → 本草稿移入 `cases/defect/auto-postgres-c8d36e9a2f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
