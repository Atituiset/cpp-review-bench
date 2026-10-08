# auto-postgres-b3f8cbb083

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#ce3387b74b708a679282ba5c72990b532d661bb8](https://github.com/postgres/postgres/commit/ce3387b74b708a679282ba5c72990b532d661bb8) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-08 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 13 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #ce3387b74b708a679282ba5c72990b532d661bb8 (https://github.com/postgres/postgres/commit/ce3387b74b708a679282ba5c72990b532d661bb8)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 2180；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT ce3387b74b708a679282ba5c72990b532d661bb8 Reuse zstd decompression contexts when restoring WAL images :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -36,6 +36,7 @@
 #ifndef FRONTEND
 #include "pgstat.h"
 #include "storage/bufmgr.h"
+#include "utils/memutils.h"
 #include "utils/wait_event.h"
 #else
 #include "common/logging.h"
@@ -55,6 +56,9 @@ static bool ValidXLogRecord(XLogReaderState *state, XLogRecord *record,
 static void ResetDecoder(XLogReaderState *state);
 static void WALOpenSegmentInit(WALOpenSegment *seg, WALSegmentContext *segcxt,
 							   int segsize, const char *waldir);
+#if defined(USE_ZSTD) && !defined(FRONTEND)
+static void XLogReaderFreeZstdContext(void *arg);
+#endif
 
 /* size of the buffer allocated for error message. */
 #define MAX_ERRORMSG_LEN 1000
@@ -171,10 +175,29 @@ XLogReaderFree(XLogReaderState *state)
 	pfree(state->errormsg_buf);
 	if (state->readRecordBuf)
 		pfree(state->readRecordBuf);
+#ifdef USE_ZSTD
+	if (state->zstd_dctx != NULL)
+	{
+#ifndef FRONTEND
+		MemoryContextUnregisterResetCallback(GetMemoryChunkContext(state),
+											 &state->zstd_dctx_cb);
+#endif
+		ZSTD_freeDCtx(state->zstd_dctx);
+	}
+#endif
 	pfree(state->readBuf);
 	pfree(state);
 }
 
+#if defined(USE_ZSTD) && !defined(FRONTEND)
+/* Release the zstd decomoression context. */
+static void
+XLogReaderFreeZstdContext(void *arg)
+{
+	ZSTD_freeDCtx(arg);
+}
+#endif
+
 /*
  * Allocate readRecordBuf to fit a record of at least the given length.
  *
@@ -2177,9 +2200,31 @@ RestoreBlockImage(XLogReaderState *record, uint8 block_id, char *page)
 		else if ((bkpb->bimg_info & BKPIMAGE_COMPRESS_ZSTD) != 0)
 		{
 #ifdef USE_ZSTD
-			size_t		decomp_result = ZSTD_decompress(tmp.data,
-														BLCKSZ - bkpb->hole_length,
-														ptr, bkpb->bimg_len);
+			size_t		decomp_result;
+
+			if (record->zstd_dctx == NULL)
+			{
+				record->zstd_dctx = ZSTD_createDCtx();
+				if (record->zstd_dctx == NULL)
+				{
+					report_invalid_record(record, "out of memory while restoring image at %X/%08X, block %d",
+										  LSN_FORMAT_ARGS(record->ReadRecPtr),
+										  block_id);
+					return false;
+				}
+
+#ifndef FRONTEND
+				/* The reader may outlive the current memory context. */
+				record->zstd_dctx_cb.func = XLogReaderFreeZstdContext;
+				record->zstd_dctx_cb.arg = record->zstd_dctx;
+				MemoryContextRegisterResetCallback(GetMemoryChunkContext(record),
+												   &record->zstd_dctx_cb);
+#endif
+			}
+
+			decomp_result = ZSTD_decompressDCtx(record->zstd_dctx, tmp.data,
+												BLCKSZ - bkpb->hole_length,
+												ptr, bkpb->bimg_len);
 
 			if (ZSTD_isError(decomp_result))
 				decomp_success = false;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`pfree`
- 外部函数：`snprintf`
- 大写宏：`BKPIMAGE_COMPRESS_ZSTD`
- 大写宏：`BLCKSZ`
- 大写宏：`FRONTEND`
- 大写宏：`MAXPGPATH`
- 大写宏：`NULL`
- 大写宏：`USE_ZSTD`
- 外部类型：`Allocate`
- 外部类型：`Clear`
- 外部类型：`DecodedXLogRecord`
- 外部类型：`Reset`
- 外部类型：`WALOpenSegment`
- 外部类型：`WALSegmentContext`
- 外部类型：`XLogReaderState`
- 外部类型：`size_t`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-b3f8cbb083` → 本草稿移入 `cases/defect/auto-postgres-b3f8cbb083/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
