# auto-postgres-52397f5d92

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#a999971331301a4a2f945220733b11af90cc2962](https://github.com/postgres/postgres/commit/a999971331301a4a2f945220733b11af90cc2962) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-16 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 58 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #a999971331301a4a2f945220733b11af90cc2962 (https://github.com/postgres/postgres/commit/a999971331301a4a2f945220733b11af90cc2962)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 471；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT a999971331301a4a2f945220733b11af90cc2962 Use StringInfo to build the result in oidvectortypes() :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -20,7 +20,9 @@
 #include "access/htup_details.h"
 #include "catalog/namespace.h"
 #include "catalog/pg_type.h"
+#include "lib/stringinfo.h"
 #include "mb/pg_wchar.h"
+#include "miscadmin.h"
 #include "utils/builtins.h"
 #include "utils/fmgroids.h"
 #include "utils/lsyscache.h"
@@ -447,42 +449,30 @@ Datum
 oidvectortypes(PG_FUNCTION_ARGS)
 {
 	oidvector  *oidArray = (oidvector *) PG_GETARG_POINTER(0);
-	char	   *result;
+	StringInfoData result;
 	int			numargs;
 	int			num;
-	size_t		total;
-	size_t		left;
 
 	/* validate input before fetching dim1 */
 	check_valid_oidvector(oidArray);
 	numargs = oidArray->dim1;
 
-	total = 20 * numargs + 1;
-	result = palloc(total);
-	result[0] = '\0';
-	left = total - 1;
+	initStringInfo(&result);
 
 	for (num = 0; num < numargs; num++)
 	{
-		char	   *typename = format_type_extended(oidArray->values[num], -1,
-													FORMAT_TYPE_ALLOW_INVALID);
-		size_t		slen = strlen(typename);
+		char	   *typename;
 
-		if (left < (slen + 2))
-		{
-			total += slen + 2;
-			result = repalloc(result, total);
-			left += slen + 2;
-		}
+		CHECK_FOR_INTERRUPTS();
+
+		typename = format_type_extended(oidArray->values[num], -1,
+										FORMAT_TYPE_ALLOW_INVALID);
 
 		if (num > 0)
-		{
-			strcat(result, ", ");
-			left -= 2;
-		}
-		strcat(result, typename);
-		left -= slen;
+			appendStringInfoString(&result, ", ");
+		appendStringInfoString(&result, typename);
+		pfree(typename);
 	}
 
-	PG_RETURN_TEXT_P(cstring_to_text(result));
+	PG_RETURN_TEXT_P(cstring_to_text_with_len(result.data, result.len));
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`check_valid_oidvector`
- 外部函数：`cstring_to_text`
- 外部函数：`elog`
- 外部函数：`get_namespace_name_or_temp`
- 外部函数：`palloc`
- 外部函数：`psprintf`
- 外部函数：`pstrdup`
- 外部函数：`quote_qualified_identifier`
- 外部函数：`repalloc`
- 外部函数：`s`
- 外部函数：`strcat`
- 外部函数：`strlen`
- 大写宏：`BIT`
- 大写宏：`BITOID`
- 大写宏：`BOOLOID`
- 大写宏：`BPCHAROID`
- 大写宏：`CHARACTER`
- 大写宏：`ERROR`
- 大写宏：`FLOAT4OID`
- 大写宏：`FLOAT8OID`
- 大写宏：`FORMAT_TYPE_ALLOW_INVALID`
- 大写宏：`FORMAT_TYPE_FORCE_QUALIFY`
- 大写宏：`FORMAT_TYPE_INVALID_AS_NULL`
- 大写宏：`FORMAT_TYPE_TYPEMOD_GIVEN`
- 大写宏：`GETSTRUCT`
- 大写宏：`INT2OID`
- 大写宏：`INT4OID`
- 大写宏：`INT8OID`
- 大写宏：`INTERVALOID`
- 大写宏：`JSONOID`
- 大写宏：`NULL`
- 大写宏：`NUMERICOID`
- 大写宏：`PG_FUNCTION_ARGS`
- 大写宏：`PG_GETARG_POINTER`
- 大写宏：`PG_RETURN_TEXT_P`
- 大写宏：`SQL`
- 大写宏：`TIMEOID`
- 大写宏：`TIMESTAMPOID`
- 大写宏：`TIMESTAMPTZOID`
- 大写宏：`TIMETZOID`
- 大写宏：`TYPEOID`
- 大写宏：`TYPSTORAGE_PLAIN`
- 大写宏：`VARBITOID`
- 大写宏：`VARCHAROID`
- 外部类型：`Also`
- 外部类型：`Check`
- 外部类型：`Default`
- 外部类型：`Form_pg_type`
- 外部类型：`HeapTuple`
- 外部类型：`Here`
- 外部类型：`If`
- 外部类型：`InvalidOid`
- 外部类型：`Note`
- 外部类型：`Oid`
- 外部类型：`Pseudo`
- 外部类型：`Report`
- 外部类型：`See`
- 外部类型：`Shouldn`
- 外部类型：`Switch`
- 外部类型：`This`
- 外部类型：`Use`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-52397f5d92` → 本草稿移入 `cases/defect/auto-postgres-52397f5d92/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
