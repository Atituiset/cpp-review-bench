# auto-postgres-e16fcacf73

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#1a846a555afb0e5f2008f3aefe8f77acbe6ffba6](https://github.com/postgres/postgres/commit/1a846a555afb0e5f2008f3aefe8f77acbe6ffba6) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-09-27 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 109 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #1a846a555afb0e5f2008f3aefe8f77acbe6ffba6 (https://github.com/postgres/postgres/commit/1a846a555afb0e5f2008f3aefe8f77acbe6ffba6)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 1a846a555afb0e5f2008f3aefe8f77acbe6ffba6 Check EXECUTE privilege on functions invoked by the RI fast path :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -32,11 +32,13 @@
 #include "access/tableam.h"
 #include "access/xact.h"
 #include "catalog/index.h"
+#include "catalog/objectaccess.h"
 #include "catalog/pg_am_d.h"
 #include "catalog/pg_collation.h"
 #include "catalog/pg_constraint.h"
 #include "catalog/pg_index.h"
 #include "catalog/pg_namespace.h"
+#include "catalog/pg_proc.h"
 #include "commands/trigger.h"
 #include "executor/executor.h"
 #include "executor/spi.h"
@@ -399,6 +401,8 @@ static void ri_CheckPermissions(const RI_ConstraintInfo *riinfo,
 								Relation query_rel);
 static bool recheck_matched_pk_tuple(Relation idxrel, ScanKeyData *skeys,
 									 int nkeys, TupleTableSlot *new_slot);
+static void ri_CheckFunctionPermissions(const RI_ConstraintInfo *riinfo,
+										const FastPathMeta *fpmeta);
 static void build_index_scankeys(const RI_ConstraintInfo *riinfo,
 								 FastPathMeta *fpmeta,
 								 Relation idx_rel, Datum *pk_vals,
@@ -2971,6 +2975,7 @@ ri_FastPathCheck(RI_ConstraintInfo *riinfo,
 		ri_populate_fastpath_metadata(riinfo, fk_rel, idx_rel);
 	}
 	Assert(riinfo->fpmeta);
+	ri_CheckFunctionPermissions(riinfo, riinfo->fpmeta);
 	ri_ExtractValues(fk_rel, newslot, riinfo, false, pk_vals, pk_nulls);
 	build_index_scankeys(riinfo, riinfo->fpmeta, idx_rel, pk_vals, pk_nulls,
 						 skey);
@@ -3715,6 +3720,41 @@ recheck_matched_pk_tuple(Relation idxrel, ScanKeyData *skeys, int nkeys,
 	return matched;
 }
 
+/*
+ * ri_CheckFunctionPermissions
+ *		Check EXECUTE privilege on the functions the fast path invokes on the
+ *		FK values, as the referenced table's owner.
+ *
+ * This parallels the checks ExecInitFunc() performs when the SPI path
+ * initializes its generated query, where the equality operator's function
+ * appears in the WHERE clause and the cast function, if any, in the cast
+ * applied to the parameter.  Call with the user id already switched to the
+ * referenced table's owner.
+ */
+static void
+ri_CheckFunctionPermissions(const RI_ConstraintInfo *riinfo,
+							const FastPathMeta *fpmeta)
+{
+	for (int i = 0; i < riinfo->nkeys; i++)
+	{
+		Oid			funcs[2] = {fpmeta->regops[i], fpmeta->cast_func_finfo[i].fn_oid};
+
+		for (int j = 0; j < lengthof(funcs); j++)
+		{
+			AclResult	aclresult;
+
+			if (!OidIsValid(funcs[j]))
+				continue;
+			aclresult = object_aclcheck(ProcedureRelationId, funcs[j],
+										GetUserId(), ACL_EXECUTE);
+			if (aclresult != ACLCHECK_OK)
+				aclcheck_error(aclresult, OBJECT_FUNCTION,
+							   get_func_name(funcs[j]));
+			InvokeFunctionExecuteHook(funcs[j]);
+		}
+	}
+}
+
 /*
  * build_index_scankeys
  *		Build ScanKeys for a direct index probe of the PK's unique index.
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`anyrange`
- 外部函数：`attnumTypeId`
- 外部函数：`dclist_container`
- 外部函数：`dclist_count`
- 外部函数：`dclist_delete_from`
- 外部函数：`entry`
- 外部函数：`local`
- 外部函数：`operators`
- 外部函数：`range_agg`
- 外部函数：`recheck_matched_pk_tuple`
- 外部函数：`ri_FastPathCheck`
- 大写宏：`ALTER`
- 大写宏：`AMOPOPID`
- 大写宏：`DDL`
- 大写宏：`DELETE`
- 大写宏：`INDEX_MAX_KEYS`
- 大写宏：`NULL`
- 大写宏：`OID`
- 大写宏：`PERIOD`
- 大写宏：`RI_FASTPATH_UNKNOWN`
- 大写宏：`RI_FASTPATH_UNUSABLE`
- 大写宏：`RI_FASTPATH_USABLE`
- 大写宏：`RI_PLAN_XXX`
- 大写宏：`SET`
- 大写宏：`UPDATE`
- 外部类型：`Being`
- 外部类型：`Datum`
- 外部类型：`Detach`
- 外部类型：`FastPathMeta`
- 外部类型：`FmgrInfo`
- 外部类型：`Freeing`
- 外部类型：`If`
- 外部类型：`Link`
- 外部类型：`NameData`
- 外部类型：`Oid`
- 外部类型：`Queue`
- 外部类型：`RI_CompareHashEntry`
- 外部类型：`RI_CompareKey`
- 外部类型：`RI_ConstraintInfo`
- 外部类型：`RI_QueryHashEntry`
- 外部类型：`RI_QueryKey`
- 外部类型：`Relation`
- 外部类型：`Remove`
- 外部类型：`SPIPlanPtr`
- 外部类型：`ScanKeyData`
- 外部类型：`SysCacheIdentifier`
- 外部类型：`TABLEs`
- 外部类型：`This`
- 外部类型：`TupleTableSlot`
- 外部类型：`We`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-e16fcacf73` → 本草稿移入 `cases/defect/auto-postgres-e16fcacf73/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
