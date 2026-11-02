# auto-postgres-c5f09cf131

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#01131ca43bbf798be6ec237a33f6bef67bede888](https://github.com/postgres/postgres/commit/01131ca43bbf798be6ec237a33f6bef67bede888) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-02 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 110 |
| 编译错误数（gcc syntax-only） | 92（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #01131ca43bbf798be6ec237a33f6bef67bede888 (https://github.com/postgres/postgres/commit/01131ca43bbf798be6ec237a33f6bef67bede888)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 820；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 01131ca43bbf798be6ec237a33f6bef67bede888 Fix double-free when XMLSERIALIZE(... INDENT) output exceeds 1GB. :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -733,6 +733,7 @@ xmltotext_with_options(xmltype *data, XmlOptionType xmloption_arg, bool indent)
 	PG_TRY();
 	{
 		size_t		decl_len = 0;
+		int			close_result;
 
 		xmlerrcxt = pg_xml_init(PG_XML_STRICTNESS_ALL);
 
@@ -817,12 +818,12 @@ xmltotext_with_options(xmltype *data, XmlOptionType xmloption_arg, bool indent)
 			xmlFreeNode(newline);
 		}
 
-		if (xmlSaveClose(ctxt) == -1 || xmlerrcxt->err_occurred)
-		{
-			ctxt = NULL;		/* don't try to close it again */
+		close_result = xmlSaveClose(ctxt);
+		/* xmlSaveClose frees ctxt whether or not it succeeds */
+		ctxt = NULL;			/* don't try to close it again */
+		if (close_result == -1 || xmlerrcxt->err_occurred)
 			xml_ereport(xmlerrcxt, ERROR, ERRCODE_INTERNAL_ERROR,
 						"could not close xmlSaveCtxtPtr");
-		}
 
 		/*
 		 * xmlDocContentDumpOutput may add a trailing newline, so remove that.
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`elog`
- 外部函数：`ereport`
- 外部函数：`errcode`
- 外部函数：`errdetail`
- 外部函数：`errdetail_internal`
- 外部函数：`errhint`
- 外部函数：`errmsg`
- 外部函数：`errmsg_internal`
- 外部函数：`initStringInfo`
- 外部函数：`palloc_object`
- 外部函数：`pfree`
- 外部函数：`repalloc`
- 外部函数：`separately`
- 外部函数：`xmlFree`
- 外部函数：`xmlFreeNode`
- 外部函数：`xmlGetExternalEntityLoader`
- 外部函数：`xmlMemSetup`
- 外部函数：`xmlNewStringInputStream`
- 外部函数：`xmlSaveClose`
- 外部函数：`xmlSetExternalEntityLoader`
- 外部函数：`xmlSetStructuredErrorFunc`
- 大写宏：`ABI`
- 大写宏：`ALLOCSET_DEFAULT_SIZES`
- 大写宏：`ERRCODE_FEATURE_NOT_SUPPORTED`
- 大写宏：`ERRCODE_INTERNAL_ERROR`
- 大写宏：`ERROR`
- 大写宏：`LIBXML_TEST_VERSION`
- 大写宏：`NULL`
- 大写宏：`PG_TRY`
- 大写宏：`PG_XML_STRICTNESS_ALL`
- 大写宏：`URL`
- 大写宏：`USE_LIBXMLCONTEXT`
- 大写宏：`UTF`
- 大写宏：`XML`
- 大写宏：`XML_ELEMENT_NODE`
- 外部类型：`Also`
- 外部类型：`At`
- 外部类型：`Check`
- 外部类型：`Create`
- 外部类型：`Currently`
- 外部类型：`Defend`
- 外部类型：`Do`
- 外部类型：`Flag`
- 外部类型：`If`
- 外部类型：`Include`
- 外部类型：`Libxml`
- 外部类型：`LibxmlContext`
- 外部类型：`PgXmlErrorContext`
- 外部类型：`PgXmlErrorPtr`
- 外部类型：`PgXmlStrictness`
- 外部类型：`PostgreSQL`
- 外部类型：`Re`
- 外部类型：`Save`
- 外部类型：`Set`
- 外部类型：`StringInfoData`
- 外部类型：`Stuff`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`TopMemoryContext`
- 外部类型：`URLs`
- 外部类型：`Verify`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-c5f09cf131` → 本草稿移入 `cases/defect/auto-postgres-c5f09cf131/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
