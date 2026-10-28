# auto-postgres-5422f4d992

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#4a92c8c29bd0068ecc747cca5d77949660b5f0a8](https://github.com/postgres/postgres/commit/4a92c8c29bd0068ecc747cca5d77949660b5f0a8) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-28 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 91 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #4a92c8c29bd0068ecc747cca5d77949660b5f0a8 (https://github.com/postgres/postgres/commit/4a92c8c29bd0068ecc747cca5d77949660b5f0a8)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 8（原始 PR diff 行 343；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 4a92c8c29bd0068ecc747cca5d77949660b5f0a8 Avoid torn copy of the control file in base backups :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -23,9 +23,11 @@
 #include "backup/basebackup_incremental.h"
 #include "backup/basebackup_sink.h"
 #include "backup/basebackup_target.h"
+#include "catalog/pg_control.h"
 #include "catalog/pg_tablespace_d.h"
 #include "commands/defrem.h"
 #include "common/compression.h"
+#include "common/controldata_utils.h"
 #include "common/file_perm.h"
 #include "common/file_utils.h"
 #include "lib/stringinfo.h"
@@ -42,6 +44,7 @@
 #include "storage/checksum.h"
 #include "storage/dsm_impl.h"
 #include "storage/ipc.h"
+#include "storage/lwlock.h"
 #include "storage/reinit.h"
 #include "utils/builtins.h"
 #include "utils/guc.h"
@@ -340,9 +343,11 @@ perform_base_backup(basebackup_options *opt, bbsink *sink,
 
 			if (ti->path == NULL)
 			{
-				struct stat statbuf;
 				bool		sendtblspclinks = true;
 				char	   *backup_label;
+				ControlFileData *controlData;
+				bool		crc_ok;
+				char		controlFile[PG_CONTROL_FILE_SIZE];
 
 				bbsink_begin_archive(sink, "base.tar");
 
@@ -365,14 +370,22 @@ perform_base_backup(basebackup_options *opt, bbsink *sink,
 						sendtblspclinks, &manifest, InvalidOid, ib);
 
 				/* ... and pg_control after everything else. */
-				if (lstat(XLOG_CONTROL_FILE, &statbuf) != 0)
+				LWLockAcquire(ControlFileLock, LW_SHARED);
+				controlData = get_controlfile(DataDir, &crc_ok);
+				LWLockRelease(ControlFileLock);
+
+				if (!crc_ok)
 					ereport(ERROR,
-							(errcode_for_file_access(),
-							 errmsg("could not stat file \"%s\": %m",
-									XLOG_CONTROL_FILE)));
-				sendFile(sink, XLOG_CONTROL_FILE, XLOG_CONTROL_FILE, &statbuf,
-						 false, InvalidOid, InvalidOid,
-						 InvalidRelFileNumber, 0, &manifest, 0, NULL, 0);
+							(errmsg("calculated CRC checksum does not match value stored in file")));
+
+				/* Send the full file, zero-padded as it is on disk */
+				memset(controlFile, 0, PG_CONTROL_FILE_SIZE);
+				memcpy(controlFile, controlData, sizeof(ControlFileData));
+				pfree(controlData);
+
+				sendFileWithContent(sink, XLOG_CONTROL_FILE,
+									controlFile, PG_CONTROL_FILE_SIZE,
+									&manifest);
 			}
 			else
 			{
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`_tarWriteHeader`
- 外部函数：`backup_checksums_verifiable`
- 外部函数：`bbsink_archive_contents`
- 外部函数：`bbsink_begin_archive`
- 外部函数：`elog`
- 外部函数：`ereport`
- 外部函数：`errcode_for_file_access`
- 外部函数：`errmsg`
- 外部函数：`fstat`
- 外部函数：`lstat`
- 外部函数：`memset`
- 外部函数：`pg_checksum_init`
- 外部函数：`pg_checksum_update`
- 外部函数：`push_to_sink`
- 外部函数：`rename`
- 大写宏：`BLCKSZ`
- 大写宏：`ENOENT`
- 大写宏：`ERROR`
- 大写宏：`INCREMENTAL_MAGIC`
- 大写宏：`NULL`
- 大写宏：`O_RDONLY`
- 大写宏：`PG_BINARY`
- 大写宏：`WAL`
- 大写宏：`XLOG_CONTROL_FILE`
- 外部类型：`Add`
- 外部类型：`BlockNumber`
- 外部类型：`Checksums`
- 外部类型：`Determine`
- 外部类型：`Emit`
- 外部类型：`Flush`
- 外部类型：`If`
- 外部类型：`InvalidOid`
- 外部类型：`InvalidRelFileNumber`
- 外部类型：`It`
- 外部类型：`Loop`
- 外部类型：`Non`
- 外部类型：`Oid`
- 外部类型：`Read`
- 外部类型：`RelFileNumber`
- 外部类型：`Reuse`
- 外部类型：`The`
- 外部类型：`Update`
- 外部类型：`Verify`
- 外部类型：`off_t`
- 外部类型：`pgoff_t`
- 外部类型：`size_t`
- 外部类型：`stat`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-5422f4d992` → 本草稿移入 `cases/defect/auto-postgres-5422f4d992/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
