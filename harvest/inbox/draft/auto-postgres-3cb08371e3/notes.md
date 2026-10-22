# auto-postgres-3cb08371e3

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#5ffb8c7409385c58e5c1abaa22ee77ba4c1b5c33](https://github.com/postgres/postgres/commit/5ffb8c7409385c58e5c1abaa22ee77ba4c1b5c33) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-22 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 12 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #5ffb8c7409385c58e5c1abaa22ee77ba4c1b5c33 (https://github.com/postgres/postgres/commit/5ffb8c7409385c58e5c1abaa22ee77ba4c1b5c33)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 160；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 5ffb8c7409385c58e5c1abaa22ee77ba4c1b5c33 Use pg_parse_lsn() for timeline history switchpoints :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -39,6 +39,7 @@
 #include "access/xlog_internal.h"
 #include "access/xlogarchive.h"
 #include "access/xlogdefs.h"
+#include "common/pg_parse_lsn.h"
 #include "pgstat.h"
 #include "storage/fd.h"
 #include "utils/wait_event.h"
@@ -128,10 +129,13 @@ readTimeLineHistory(TimeLineID targetTLI)
 		char		fline[MAXPGPATH];
 		char	   *res;
 		char	   *ptr;
+		char	   *token_end;
+		char		save;
 		TimeLineID	tli;
-		uint32		switchpoint_hi;
-		uint32		switchpoint_lo;
-		int			nfields;
+		XLogRecPtr	switchpoint;
+		bool		valid_switchpoint = false;
+		int			nchars;
+		size_t		nspaces;
 
 		pgstat_report_wait_start(WAIT_EVENT_TIMELINE_HISTORY_READ);
 		res = fgets(fline, sizeof(fline), fd);
@@ -155,16 +159,33 @@ readTimeLineHistory(TimeLineID targetTLI)
 		if (*ptr == '\0' || *ptr == '#')
 			continue;
 
-		nfields = sscanf(fline, "%u\t%X/%08X", &tli, &switchpoint_hi, &switchpoint_lo);
-
-		if (nfields < 1)
+		if (sscanf(fline, "%u%n", &tli, &nchars) != 1)
 		{
 			/* expect a numeric timeline ID as first field of line */
 			ereport(FATAL,
 					(errmsg("syntax error in history file: %s", fline),
 					 errhint("Expected a numeric timeline ID.")));
 		}
-		if (nfields != 3)
+
+		/* the switchpoint location follows, separated by whitespace */
+		ptr = fline + nchars;
+		nspaces = strspn(ptr, " \t\n\r\f\v");
+		if (nspaces > 0)
+		{
+			ptr += nspaces;
+
+			/*
+			 * isolate the location from the rest of the line before parsing
+			 * it
+			 */
+			token_end = ptr + strcspn(ptr, " \t\n\r\f\v");
+			save = *token_end;
+			*token_end = '\0';
+			valid_switchpoint = pg_parse_lsn(ptr, &switchpoint);
+			*token_end = save;
+		}
+
+		if (!valid_switchpoint)
 			ereport(FATAL,
 					(errmsg("syntax error in history file: %s", fline),
 					 errhint("Expected a write-ahead log switchpoint location.")));
@@ -179,7 +200,7 @@ readTimeLineHistory(TimeLineID targetTLI)
 		entry = palloc_object(TimeLineHistoryEntry);
 		entry->tli = tli;
 		entry->begin = prevend;
-		entry->end = ((uint64) (switchpoint_hi)) << 32 | (uint64) switchpoint_lo;
+		entry->end = switchpoint;
 		prevend = entry->end;
 
 		/* Build list with newest item first */
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`ereport`
- 外部函数：`errhint`
- 外部函数：`errmsg`
- 外部函数：`fgets`
- 外部函数：`palloc_object`
- 外部函数：`pgstat_report_wait_start`
- 外部函数：`sscanf`
- 大写宏：`FATAL`
- 大写宏：`MAXPGPATH`
- 大写宏：`WAIT_EVENT_TIMELINE_HISTORY_READ`
- 外部类型：`Build`
- 外部类型：`Expected`
- 外部类型：`TimeLineHistoryEntry`
- 外部类型：`TimeLineID`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-3cb08371e3` → 本草稿移入 `cases/defect/auto-postgres-3cb08371e3/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
