# auto-postgres-a83cd9fdc3

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#ba6d9ee259cdb2a6c862a4741b16194106e00a9a](https://github.com/postgres/postgres/commit/ba6d9ee259cdb2a6c862a4741b16194106e00a9a) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-23 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 7 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #ba6d9ee259cdb2a6c862a4741b16194106e00a9a (https://github.com/postgres/postgres/commit/ba6d9ee259cdb2a6c862a4741b16194106e00a9a)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 461；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT ba6d9ee259cdb2a6c862a4741b16194106e00a9a Use pg_parse_lsn() for IDENTIFY_SYSTEM responses in libpqwalreceiver :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -23,6 +23,7 @@
 #include <sys/time.h>
 
 #include "common/connect.h"
+#include "common/pg_parse_lsn.h"
 #include "funcapi.h"
 #include "libpq-fe.h"
 #include "libpq/libpq-be-fe-helpers.h"
@@ -458,15 +459,11 @@ libpqrcv_identify_system(WalReceiverConn *conn, TimeLineID *primary_tli,
 	/* Column 2 is the server's current WAL flush position */
 	if (server_lsn)
 	{
-		uint32		hi,
-					lo;
-
-		if (sscanf(PQgetvalue(res, 0, 2), "%X/%X", &hi, &lo) != 2)
+		if (!pg_parse_lsn(PQgetvalue(res, 0, 2), server_lsn))
 			ereport(ERROR,
 					(errcode(ERRCODE_PROTOCOL_VIOLATION),
 					 errmsg("could not parse WAL location \"%s\"",
 							PQgetvalue(res, 0, 2))));
-		*server_lsn = ((uint64) hi) << 32 | lo;
 	}
 
 	PQclear(res);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`ereport`
- 外部函数：`errcode`
- 外部函数：`errmsg`
- 外部函数：`sscanf`
- 大写宏：`ERRCODE_PROTOCOL_VIOLATION`
- 大写宏：`ERROR`
- 大写宏：`WAL`
- 外部类型：`Column`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-a83cd9fdc3` → 本草稿移入 `cases/defect/auto-postgres-a83cd9fdc3/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
