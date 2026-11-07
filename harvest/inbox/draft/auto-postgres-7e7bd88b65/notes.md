# auto-postgres-7e7bd88b65

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#b58a9a97fe783c8ce897fa1a5da23c59234aa6bf](https://github.com/postgres/postgres/commit/b58a9a97fe783c8ce897fa1a5da23c59234aa6bf) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-07 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 72 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #b58a9a97fe783c8ce897fa1a5da23c59234aa6bf (https://github.com/postgres/postgres/commit/b58a9a97fe783c8ce897fa1a5da23c59234aa6bf)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 6129；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT b58a9a97fe783c8ce897fa1a5da23c59234aa6bf Revert "make immutability tests in to_json and to_jsonb complete" :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -17,8 +17,6 @@
 #include <limits.h>
 
 #include "access/htup_details.h"
-#include "access/tupdesc.h"
-#include "catalog/pg_proc.h"
 #include "catalog/pg_type.h"
 #include "common/int.h"
 #include "common/jsonapi.h"
@@ -6085,106 +6083,3 @@ json_categorize_type(Oid typoid, bool is_jsonb,
 			break;
 	}
 }
-
-/*
- * Check whether a type conversion to JSON or JSONB involves any mutable
- * functions.  This recurses into container types (arrays, composites,
- * ranges, multiranges, domains) to check their element/sub types.
- *
- * The caller must initialize *has_mutable to false before calling.
- * If any mutable function is found, *has_mutable is set to true.
- */
-void
-json_check_mutability(Oid typoid, bool is_jsonb, bool *has_mutable)
-{
-	char		att_typtype = get_typtype(typoid);
-	JsonTypeCategory tcategory;
-	Oid			outfuncoid;
-
-	/* since this function recurses, it could be driven to stack overflow */
-	check_stack_depth();
-
-	Assert(has_mutable != NULL);
-
-	if (*has_mutable)
-		return;
-
-	if (att_typtype == TYPTYPE_DOMAIN)
-	{
-		json_check_mutability(getBaseType(typoid), is_jsonb, has_mutable);
-		return;
-	}
-	else if (att_typtype == TYPTYPE_COMPOSITE)
-	{
-		/*
-		 * For a composite type, recurse into its attributes.  Use the
-		 * typcache to avoid opening the relation directly.
-		 */
-		TupleDesc	tupdesc = lookup_rowtype_tupdesc(typoid, -1);
-
-		for (int i = 0; i < tupdesc->natts; i++)
-		{
-			Form_pg_attribute attr = TupleDescAttr(tupdesc, i);
-
-			if (attr->attisdropped)
-				continue;
-
-			json_check_mutability(attr->atttypid, is_jsonb, has_mutable);
-			if (*has_mutable)
-				break;
-		}
-		ReleaseTupleDesc(tupdesc);
-		return;
-	}
-	else if (att_typtype == TYPTYPE_RANGE)
-	{
-		json_check_mutability(get_range_subtype(typoid), is_jsonb,
-							  has_mutable);
-		return;
-	}
-	else if (att_typtype == TYPTYPE_MULTIRANGE)
-	{
-		json_check_mutability(get_multirange_range(typoid), is_jsonb,
-							  has_mutable);
-		return;
-	}
-	else
-	{
-		Oid			att_typelem = get_element_type(typoid);
-
-		if (OidIsValid(att_typelem))
-		{
-			/* recurse into array element type */
-			json_check_mutability(att_typelem, is_jsonb, has_mutable);
-			return;
-		}
-	}
-
-	json_categorize_type(typoid, is_jsonb, &tcategory, &outfuncoid);
-
-	switch (tcategory)
-	{
-		case JSONTYPE_NULL:
-		case JSONTYPE_BOOL:
-		case JSONTYPE_NUMERIC:
-			break;
-
-		case JSONTYPE_DATE:
-		case JSONTYPE_TIMESTAMP:
-		case JSONTYPE_TIMESTAMPTZ:
-			*has_mutable = true;
-			break;
-
-		case JSONTYPE_JSON:
-		case JSONTYPE_JSONB:
-		case JSONTYPE_ARRAY:
-		case JSONTYPE_COMPOSITE:
-			break;
-
-		case JSONTYPE_CAST:
-		case JSONTYPE_OTHER:
-			if (func_volatile(outfuncoid) != PROVOLATILE_IMMUTABLE)
-				*has_mutable = true;
-			break;
-	}
-}
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`check_stack_depth`
- 外部函数：`find_coercion_pathway`
- 外部函数：`func_volatile`
- 外部函数：`getBaseType`
- 外部函数：`getTypeOutputInfo`
- 外部函数：`get_element_type`
- 外部函数：`get_multirange_range`
- 外部函数：`get_range_subtype`
- 外部函数：`get_typtype`
- 外部函数：`json`
- 外部函数：`json_check_mutability`
- 外部函数：`lookup_rowtype_tupdesc`
- 外部函数：`type_is_rowtype`
- 大写宏：`ANYARRAYOID`
- 大写宏：`ANYCOMPATIBLEARRAYOID`
- 大写宏：`BOOLOID`
- 大写宏：`COERCION_EXPLICIT`
- 大写宏：`COERCION_PATH_FUNC`
- 大写宏：`DATEOID`
- 大写宏：`FLOAT4OID`
- 大写宏：`FLOAT8OID`
- 大写宏：`F_ARRAY_OUT`
- 大写宏：`F_BOOLOUT`
- 大写宏：`F_DATE_OUT`
- 大写宏：`F_RECORD_OUT`
- 大写宏：`F_TIMESTAMPTZ_OUT`
- 大写宏：`F_TIMESTAMP_OUT`
- 大写宏：`INT2OID`
- 大写宏：`INT4OID`
- 大写宏：`INT8OID`
- 大写宏：`JSON`
- 大写宏：`JSONB`
- 大写宏：`JSONBOID`
- 大写宏：`JSONOID`
- 大写宏：`JSONTYPE_ARRAY`
- 大写宏：`JSONTYPE_BOOL`
- 大写宏：`JSONTYPE_CAST`
- 大写宏：`JSONTYPE_COMPOSITE`
- 大写宏：`JSONTYPE_DATE`
- 大写宏：`JSONTYPE_JSON`
- 大写宏：`JSONTYPE_JSONB`
- 大写宏：`JSONTYPE_NULL`
- 大写宏：`JSONTYPE_NUMERIC`
- 大写宏：`JSONTYPE_OTHER`
- 大写宏：`JSONTYPE_TIMESTAMP`
- 大写宏：`JSONTYPE_TIMESTAMPTZ`
- 大写宏：`NULL`
- 大写宏：`NUMERICOID`
- 大写宏：`PROVOLATILE_IMMUTABLE`
- 大写宏：`RECORDARRAYOID`
- 大写宏：`RECORDOID`
- 大写宏：`TIMESTAMPOID`
- 大写宏：`TIMESTAMPTZOID`
- 大写宏：`TYPTYPE_COMPOSITE`
- 大写宏：`TYPTYPE_DOMAIN`
- 大写宏：`TYPTYPE_MULTIRANGE`
- 大写宏：`TYPTYPE_RANGE`
- 外部类型：`But`
- 外部类型：`Check`
- 外部类型：`CoercionPathType`
- 外部类型：`FirstNormalObjectId`
- 外部类型：`For`
- 外部类型：`Form_pg_attribute`
- 外部类型：`If`
- 外部类型：`InvalidOid`
- 外部类型：`It`
- 外部类型：`JsonTypeCategory`
- 外部类型：`Look`
- 外部类型：`Oid`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`TupleDesc`
- 外部类型：`Use`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-7e7bd88b65` → 本草稿移入 `cases/defect/auto-postgres-7e7bd88b65/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
