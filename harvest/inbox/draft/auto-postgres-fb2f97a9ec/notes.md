# auto-postgres-fb2f97a9ec

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#1e707d32564497ed2478f4186b90d0821719957b](https://github.com/postgres/postgres/commit/1e707d32564497ed2478f4186b90d0821719957b) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-22 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 34 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #1e707d32564497ed2478f4186b90d0821719957b (https://github.com/postgres/postgres/commit/1e707d32564497ed2478f4186b90d0821719957b)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 1e707d32564497ed2478f4186b90d0821719957b Reject COPY TO of unlogged tables during recovery. :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -21,6 +21,7 @@
 #include "access/table.h"
 #include "access/tableam.h"
 #include "access/tupconvert.h"
+#include "access/xlog.h"
 #include "catalog/pg_inherits.h"
 #include "commands/copyapi.h"
 #include "commands/progress.h"
@@ -877,6 +878,19 @@ BeginCopyTo(ParseState *pstate,
 							errhint("Try the COPY (SELECT ...) TO variant."));
 				}
 
+				/* Unlogged partitions are inaccessible during recovery. */
+				if (RecoveryInProgress() &&
+					get_rel_persistence(child) == RELPERSISTENCE_UNLOGGED)
+				{
+					char	   *relation_name = get_rel_name(child);
+
+					ereport(ERROR,
+							errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
+							errmsg("cannot access temporary or unlogged relations during recovery"),
+							errdetail("Partition \"%s\" is an unlogged table in partitioned table \"%s\".",
+									  relation_name, RelationGetRelationName(rel)));
+				}
+
 				/* Exclude tables with no data */
 				if (RELKIND_HAS_PARTITIONS(relkind))
 					children = foreach_delete_current(children, child);
@@ -889,6 +903,11 @@ BeginCopyTo(ParseState *pstate,
 							RelationGetRelationName(rel))));
 	}
 
+	/* Temporary and unlogged relations are inaccessible during recovery. */
+	if (rel != NULL && !RelationIsPermanent(rel) && RecoveryInProgress())
+		ereport(ERROR,
+				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
+				 errmsg("cannot access temporary or unlogged relations during recovery")));
 
 	/* Allocate workspace and zero all fields */
 	cstate = palloc0_object(CopyToStateData);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`condition`
- 外部函数：`errhint`
- 外部函数：`file`
- 外部函数：`foreach_delete_current`
- 外部函数：`palloc0_object`
- 大写宏：`ASCII`
- 大写宏：`COPY`
- 大写宏：`COPY_CALLBACK`
- 大写宏：`COPY_FILE`
- 大写宏：`COPY_FRONTEND`
- 大写宏：`FILE`
- 大写宏：`JSON`
- 大写宏：`NULL`
- 大写宏：`OID`
- 大写宏：`RELKIND_HAS_PARTITIONS`
- 大写宏：`SELECT`
- 大写宏：`STDOUT`
- 大写宏：`WHERE`
- 外部类型：`Allocate`
- 外部类型：`BeginCopyTo`
- 外部类型：`CopyFormatOptions`
- 外部类型：`CopyToRoutine`
- 外部类型：`CopyToStateData`
- 外部类型：`Datum`
- 外部类型：`Descriptor`
- 外部类型：`Exclude`
- 外部类型：`FmgrInfo`
- 外部类型：`List`
- 外部类型：`MemoryContext`
- 外部类型：`Node`
- 外部类型：`QueryDesc`
- 外部类型：`Relation`
- 外部类型：`StringInfo`
- 外部类型：`Try`
- 外部类型：`TupleDesc`
- 外部类型：`Working`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-fb2f97a9ec` → 本草稿移入 `cases/defect/auto-postgres-fb2f97a9ec/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
