# auto-postgres-0b42a032ed

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#a5ccf20801c821b72bb04d56b7320d52149a1ce5](https://github.com/postgres/postgres/commit/a5ccf20801c821b72bb04d56b7320d52149a1ce5) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-26 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 16 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #a5ccf20801c821b72bb04d56b7320d52149a1ce5 (https://github.com/postgres/postgres/commit/a5ccf20801c821b72bb04d56b7320d52149a1ce5)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 1763；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT a5ccf20801c821b72bb04d56b7320d52149a1ce5 Fix partial two-phase files left by crash during checkpoint :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -90,6 +90,7 @@
 #include "access/xlogutils.h"
 #include "catalog/pg_type.h"
 #include "catalog/storage.h"
+#include "common/string.h"
 #include "funcapi.h"
 #include "miscadmin.h"
 #include "pg_trace.h"
@@ -1750,6 +1751,7 @@ static void
 RecreateTwoPhaseFile(FullTransactionId fxid, const void *content, size_t len)
 {
 	char		path[MAXPGPATH];
+	char		tmppath[MAXPGPATH];
 	pg_crc32c	statefile_crc;
 	int			fd;
 
@@ -1760,12 +1762,15 @@ RecreateTwoPhaseFile(FullTransactionId fxid, const void *content, size_t len)
 
 	TwoPhaseFilePath(path, fxid);
 
-	fd = OpenTransientFile(path,
+	/* Write to a temp file and rename it into place */
+	snprintf(tmppath, sizeof(tmppath), "%s.tmp", path);
+
+	fd = OpenTransientFile(tmppath,
 						   O_CREAT | O_TRUNC | O_WRONLY | PG_BINARY);
 	if (fd < 0)
 		ereport(ERROR,
 				(errcode_for_file_access(),
-				 errmsg("could not recreate file \"%s\": %m", path)));
+				 errmsg("could not recreate file \"%s\": %m", tmppath)));
 
 	/* Write content and CRC */
 	errno = 0;
@@ -1777,7 +1782,7 @@ RecreateTwoPhaseFile(FullTransactionId fxid, const void *content, size_t len)
 			errno = ENOSPC;
 		ereport(ERROR,
 				(errcode_for_file_access(),
-				 errmsg("could not write file \"%s\": %m", path)));
+				 errmsg("could not write file \"%s\": %m", tmppath)));
 	}
 	if (write(fd, &statefile_crc, sizeof(pg_crc32c)) != sizeof(pg_crc32c))
 	{
@@ -1786,7 +1791,7 @@ RecreateTwoPhaseFile(FullTransactionId fxid, const void *content, size_t len)
 			errno = ENOSPC;
 		ereport(ERROR,
 				(errcode_for_file_access(),
-				 errmsg("could not write file \"%s\": %m", path)));
+				 errmsg("could not write file \"%s\": %m", tmppath)));
 	}
 	pgstat_report_wait_end();
 
@@ -1798,13 +1803,23 @@ RecreateTwoPhaseFile(FullTransactionId fxid, const void *content, size_t len)
 	if (pg_fsync(fd) != 0)
 		ereport(ERROR,
 				(errcode_for_file_access(),
-				 errmsg("could not fsync file \"%s\": %m", path)));
+				 errmsg("could not fsync file \"%s\": %m", tmppath)));
 	pgstat_report_wait_end();
 
 	if (CloseTransientFile(fd) != 0)
 		ereport(ERROR,
 				(errcode_for_file_access(),
-				 errmsg("could not close file \"%s\": %m", path)));
+				 errmsg("could not close file \"%s\": %m", tmppath)));
+
+	/*
+	 * No durable_rename(), as CheckPointTwoPhase() fsyncs the parent
+	 * directory.
+	 */
+	if (rename(tmppath, path) != 0)
+		ereport(ERROR,
+				(errcode_for_file_access(),
+				 errmsg("could not rename file \"%s\" to \"%s\": %m",
+						tmppath, path)));
 }
 
 /*
@@ -1918,6 +1933,19 @@ restoreTwoPhaseData(void)
 	cldir = AllocateDir(TWOPHASE_DIR);
 	while ((clde = ReadDir(cldir, TWOPHASE_DIR)) != NULL)
 	{
+		/* Ignore temp files created by RecreateTwoPhaseFile(). */
+		if (pg_str_endswith(clde->d_name, ".tmp"))
+		{
+			char		path[MAXPGPATH];
+
+			snprintf(path, MAXPGPATH, TWOPHASE_DIR "/%s", clde->d_name);
+			if (unlink(path) < 0)
+				ereport(WARNING,
+						(errcode_for_file_access(),
+						 errmsg("could not remove file \"%s\": %m", path)));
+			continue;
+		}
+
 		if (strlen(clde->d_name) == 16 &&
 			strspn(clde->d_name, "0123456789ABCDEF") == 16)
 		{
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`ereport`
- 外部函数：`errcode_for_file_access`
- 外部函数：`errmsg`
- 外部函数：`pg_fsync`
- 外部函数：`pgstat_report_wait_end`
- 外部函数：`snprintf`
- 外部函数：`strlen`
- 外部函数：`strspn`
- 外部函数：`write`
- 大写宏：`CRC`
- 大写宏：`ENOSPC`
- 大写宏：`ERROR`
- 大写宏：`MAXPGPATH`
- 大写宏：`NULL`
- 大写宏：`O_CREAT`
- 大写宏：`O_TRUNC`
- 大写宏：`O_WRONLY`
- 大写宏：`PG_BINARY`
- 外部类型：`FullTransactionId`
- 外部类型：`Write`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-0b42a032ed` → 本草稿移入 `cases/defect/auto-postgres-0b42a032ed/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
