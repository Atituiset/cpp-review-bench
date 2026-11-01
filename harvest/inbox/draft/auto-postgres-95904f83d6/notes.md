# auto-postgres-95904f83d6

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#77705d1e5a616339c3e92f586bbfa7b6bb562beb](https://github.com/postgres/postgres/commit/77705d1e5a616339c3e92f586bbfa7b6bb562beb) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-01 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 179 |
| 编译错误数（gcc syntax-only） | 38（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #77705d1e5a616339c3e92f586bbfa7b6bb562beb (https://github.com/postgres/postgres/commit/77705d1e5a616339c3e92f586bbfa7b6bb562beb)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 4386；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 77705d1e5a616339c3e92f586bbfa7b6bb562beb Fix two crashes on a concurrently dropped schema :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -4381,15 +4381,18 @@ pg_identify_object(PG_FUNCTION_ARGS)
 	objidentity = getObjectIdentity(&address, true);
 
 	/* schema name */
+	nulls[1] = true;
 	if (OidIsValid(schema_oid) && objidentity)
 	{
-		const char *schema = quote_identifier(get_namespace_name(schema_oid));
+		char	   *nspname = get_namespace_name(schema_oid);
 
-		values[1] = CStringGetTextDatum(schema);
-		nulls[1] = false;
+		/* The schema may have been dropped concurrently; report it as NULL */
+		if (nspname != NULL)
+		{
+			values[1] = CStringGetTextDatum(quote_identifier(nspname));
+			nulls[1] = false;
+		}
 	}
-	else
-		nulls[1] = true;
 
 	/* object name */
 	if (objname && objidentity)
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`appendStringInfo`
- 外部函数：`appendStringInfoString`
- 外部函数：`elog`
- 外部函数：`format_procedure_extended`
- 外部函数：`format_procedure_parts`
- 外部函数：`format_type_be_qualified`
- 外部函数：`format_type_extended`
- 外部函数：`getObjectIdentity`
- 外部函数：`getRelationIdentity`
- 外部函数：`get_attname`
- 外部函数：`get_namespace_name`
- 外部函数：`get_object_property_data`
- 外部函数：`get_publication_name`
- 外部函数：`heap_copytuple`
- 外部函数：`initStringInfo`
- 外部函数：`lappend`
- 外部函数：`list_make1`
- 外部函数：`pfree`
- 外部函数：`quote_identifier`
- 外部函数：`systable_beginscan`
- 外部函数：`systable_endscan`
- 外部函数：`systable_getnext`
- 外部函数：`table_close`
- 外部函数：`table_open`
- 大写宏：`ERROR`
- 大写宏：`FORMAT_PROC_FORCE_QUALIFY`
- 大写宏：`FORMAT_PROC_INVALID_AS_NULL`
- 大写宏：`FORMAT_TYPE_FORCE_QUALIFY`
- 大写宏：`FORMAT_TYPE_INVALID_AS_NULL`
- 大写宏：`F_OIDEQ`
- 大写宏：`GETSTRUCT`
- 大写宏：`NIL`
- 大写宏：`NULL`
- 大写宏：`PUBLICATIONNAMESPACE`
- 外部类型：`AccessShareLock`
- 外部类型：`Anum_pg_cast_oid`
- 外部类型：`AttrNumber`
- 外部类型：`BTEqualStrategyNumber`
- 外部类型：`CastRelationId`
- 外部类型：`Check`
- 外部类型：`For`
- 外部类型：`Form_pg_cast`
- 外部类型：`Form_pg_publication_namespace`
- 外部类型：`HeapTuple`
- 外部类型：`InplaceUpdateTupleLock`
- 外部类型：`List`
- 外部类型：`Make`
- 外部类型：`ObjectAddress`
- 外部类型：`ObjectPropertyType`
- 外部类型：`Oid`
- 外部类型：`ProcedureRelationId`
- 外部类型：`Relation`
- 外部类型：`RelationRelationId`
- 外部类型：`ScanKeyData`
- 外部类型：`StringInfoData`
- 外部类型：`SysCacheIdentifier`
- 外部类型：`SysScanDesc`
- 外部类型：`TypeRelationId`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-95904f83d6` → 本草稿移入 `cases/defect/auto-postgres-95904f83d6/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
