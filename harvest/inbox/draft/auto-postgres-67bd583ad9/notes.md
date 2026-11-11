# auto-postgres-67bd583ad9

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#c5daeb1e0f5adfbc19ed621bb97ce2d68c8455bf](https://github.com/postgres/postgres/commit/c5daeb1e0f5adfbc19ed621bb97ce2d68c8455bf) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-11 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 13 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #c5daeb1e0f5adfbc19ed621bb97ce2d68c8455bf (https://github.com/postgres/postgres/commit/c5daeb1e0f5adfbc19ed621bb97ce2d68c8455bf)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 3412；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT c5daeb1e0f5adfbc19ed621bb97ce2d68c8455bf SQL/JSON: Fix PASSING of string types other than text and varchar :: PR 修复动作推断：修复前解引用（疑似缺判空）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -73,6 +73,7 @@
 #include "utils/formatting.h"
 #include "utils/json.h"
 #include "utils/jsonpath.h"
+#include "utils/lsyscache.h"
 #include "utils/memutils.h"
 #include "utils/timestamp.h"
 
@@ -3360,14 +3361,28 @@ JsonItemFromDatum(Datum val, Oid typid, int32 typmod, JsonbValue *res)
 			break;
 		case TEXTOID:
 		case VARCHAROID:
+		case BPCHAROID:
 			{
+				/*
+				 * Note that bpchar values keep their padding, as in
+				 * to_jsonb()
+				 */
 				text	   *txt = DatumGetTextPP(val);
 
 				res->type = jbvString;
 				res->val.string.val = VARDATA_ANY(txt);
 				res->val.string.len = VARSIZE_ANY_EXHDR(txt);
 				break;
 			}
+		case NAMEOID:
+			{
+				char	   *str = NameStr(*DatumGetName(val));
+
+				res->type = jbvString;
+				res->val.string.val = str;
+				res->val.string.len = strlen(str);
+				break;
+			}
 		case DATEOID:
 		case TIMEOID:
 		case TIMETZOID:
@@ -3409,10 +3424,38 @@ JsonItemFromDatum(Datum val, Oid typid, int32 typmod, JsonbValue *res)
 				break;
 			}
 		default:
-			ereport(ERROR,
-					errcode(ERRCODE_INVALID_PARAMETER_VALUE),
-					errmsg("could not convert value of type %s to jsonpath",
-						   format_type_be(typid)));
+			{
+				char		typcategory;
+				bool		typispreferred;
+
+				/*
+				 * transformJsonValueExpr() passes values of any type of the
+				 * string category as they are, so handle those not covered
+				 * above (such as extension types) using their output
+				 * function, as to_jsonb() does.
+				 */
+				get_type_category_preferred(typid, &typcategory,
+											&typispreferred);
+				if (typcategory == TYPCATEGORY_STRING)
+				{
+					Oid			typoutput;
+					bool		typisvarlena;
+					char	   *str;
+
+					getTypeOutputInfo(typid, &typoutput, &typisvarlena);
+					str = OidOutputFunctionCall(typoutput, val);
+
+					res->type = jbvString;
+					res->val.string.val = str;
+					res->val.string.len = strlen(str);
+					break;
+				}
+
+				ereport(ERROR,
+						errcode(ERRCODE_INVALID_PARAMETER_VALUE),
+						errmsg("could not convert value of type %s to jsonpath",
+							   format_type_be(typid)));
+			}
 	}
 }
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`ereport`
- 外部函数：`errcode`
- 外部函数：`errmsg`
- 外部函数：`format_type_be`
- 大写宏：`DATEOID`
- 大写宏：`ERRCODE_INVALID_PARAMETER_VALUE`
- 大写宏：`ERROR`
- 大写宏：`TEXTOID`
- 大写宏：`TIMEOID`
- 大写宏：`TIMETZOID`
- 大写宏：`VARCHAROID`
- 大写宏：`VARDATA_ANY`
- 大写宏：`VARSIZE_ANY_EXHDR`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-67bd583ad9` → 本草稿移入 `cases/defect/auto-postgres-67bd583ad9/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
