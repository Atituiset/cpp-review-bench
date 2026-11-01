# auto-postgres-39262ca454

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
| 外部依赖数（dep_count） | 42 |
| 编译错误数（gcc syntax-only） | 9（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #77705d1e5a616339c3e92f586bbfa7b6bb562beb (https://github.com/postgres/postgres/commit/77705d1e5a616339c3e92f586bbfa7b6bb562beb)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 3243；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 77705d1e5a616339c3e92f586bbfa7b6bb562beb Fix two crashes on a concurrently dropped schema :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3236,12 +3236,22 @@ schema_to_xml_internal(Oid nspid, const char *xmlschema, bool nulls,
 					   bool tableforest, const char *targetns, bool top_level)
 {
 	StringInfo	result;
+	char	   *nspname;
 	char	   *xmlsn;
 	List	   *relid_list;
 	ListCell   *cell;
 
-	xmlsn = map_sql_identifier_to_xml_name(get_namespace_name(nspid),
-										   true, false);
+	/*
+	 * The schema may have been dropped concurrently, as we hold no lock on
+	 * it.  Report that rather than passing a NULL name below.
+	 */
+	nspname = get_namespace_name(nspid);
+	if (nspname == NULL)
+		ereport(ERROR,
+				(errcode(ERRCODE_UNDEFINED_SCHEMA),
+				 errmsg("schema with OID %u does not exist", nspid)));
+
+	xmlsn = map_sql_identifier_to_xml_name(nspname, true, false);
 	result = makeStringInfo();
 
 	xmldata_root_element_start(result, xmlsn, xmlschema, targetns, top_level);
@@ -3685,6 +3695,15 @@ map_sql_schema_to_xmlschema_types(Oid nspid, List *relid_list, bool nulls,
 	dbname = get_database_name(MyDatabaseId);
 	nspname = get_namespace_name(nspid);
 
+	/*
+	 * The schema may have been dropped concurrently, as we hold no lock on
+	 * it.  Report that rather than passing a NULL name below.
+	 */
+	if (nspname == NULL)
+		ereport(ERROR,
+				(errcode(ERRCODE_UNDEFINED_SCHEMA),
+				 errmsg("schema with OID %u does not exist", nspid)));
+
 	initStringInfo(&result);
 
 	xmlsn = map_sql_identifier_to_xml_name(nspname, true, false);
@@ -3774,11 +3793,18 @@ map_sql_catalog_to_xmlschema_types(List *nspid_list, bool nulls,
 	{
 		Oid			nspid = lfirst_oid(cell);
 		char	   *nspname = get_namespace_name(nspid);
-		char	   *xmlsn = map_sql_identifier_to_xml_name(nspname, true, false);
-		char	   *schematypename = map_multipart_sql_identifier_to_xml_name("SchemaType",
-																			  dbname,
-																			  nspname,
-																			  NULL);
+		char	   *xmlsn;
+		char	   *schematypename;
+
+		/* Skip a schema that was concurrently dropped; it was not scanned. */
+		if (nspname == NULL)
+			continue;
+
+		xmlsn = map_sql_identifier_to_xml_name(nspname, true, false);
+		schematypename = map_multipart_sql_identifier_to_xml_name("SchemaType",
+																  dbname,
+																  nspname,
+																  NULL);
 
 		appendStringInfo(&result,
 						 "    <xsd:element name=\"%s\" type=\"%s\"/>\n",
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`appendBinaryStringInfo`
- 外部函数：`appendStringInfo`
- 外部函数：`appendStringInfoString`
- 外部函数：`errcode`
- 外部函数：`errdetail`
- 外部函数：`errmsg`
- 外部函数：`get_database_name`
- 外部函数：`get_namespace_name`
- 外部函数：`initStringInfo`
- 外部函数：`lfirst_oid`
- 外部函数：`makeStringInfo`
- 外部函数：`pfree`
- 外部函数：`pg_encoding_mb2wchar_with_len`
- 外部函数：`pg_encoding_mblen`
- 外部函数：`pg_server_to_any`
- 外部函数：`pg_strncasecmp`
- 外部函数：`sqlchar_to_unicode`
- 外部函数：`strlen`
- 外部函数：`xmlIsBaseCharQ`
- 外部函数：`xmlIsCombiningQ`
- 外部函数：`xmlIsDigitQ`
- 外部函数：`xmlIsExtenderQ`
- 外部函数：`xmlIsIdeographicQ`
- 大写宏：`ERRCODE_FEATURE_NOT_SUPPORTED`
- 大写宏：`ERROR`
- 大写宏：`FIXME`
- 大写宏：`NULL`
- 大写宏：`PG_UTF8`
- 大写宏：`SQL`
- 大写宏：`USE_LIBXML`
- 大写宏：`XML`
- 外部类型：`CombiningChar`
- 外部类型：`Digit`
- 外部类型：`Extender`
- 外部类型：`Letter`
- 外部类型：`List`
- 外部类型：`ListCell`
- 外部类型：`MyDatabaseId`
- 外部类型：`Oid`
- 外部类型：`SchemaType`
- 外部类型：`StringInfo`
- 外部类型：`StringInfoData`
- 外部类型：`This`
- 外部类型：`XMLSchema`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-39262ca454` → 本草稿移入 `cases/defect/auto-postgres-39262ca454/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
