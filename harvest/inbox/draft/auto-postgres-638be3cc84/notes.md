# auto-postgres-638be3cc84

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#245eb832f88631c986a2dadad2cbe94f57089fd6](https://github.com/postgres/postgres/commit/245eb832f88631c986a2dadad2cbe94f57089fd6) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-10 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 54 |
| 编译错误数（gcc syntax-only） | 76（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #245eb832f88631c986a2dadad2cbe94f57089fd6 (https://github.com/postgres/postgres/commit/245eb832f88631c986a2dadad2cbe94f57089fd6)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 245eb832f88631c986a2dadad2cbe94f57089fd6 Introduce attribute versioning to prevent unsafe deforming :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1575,6 +1575,106 @@ untransformRelOptions(Datum options)
 	return result;
 }
 
+/*
+ * Storage for per-attribute version in the backbranches.
+ *
+ * See the "Version storage in the catalog" section in
+ * src/backend/catalog/README.attver.md.
+ */
+
+/*
+ * Extract the attribute version from an attoptions datum.  Returns
+ * InvalidXLogRecPtr if the datum is null or does not carry a version (e.g. a
+ * pg_upgrade'd or older-minor-version row).
+ */
+XLogRecPtr
+attoptions_get_version(Datum attoptions, bool isnull)
+{
+	XLogRecPtr	version = InvalidXLogRecPtr;
+	Datum	   *optiondatums;
+	int			noptions;
+
+	if (isnull || DatumGetPointer(attoptions) == NULL)
+		return InvalidXLogRecPtr;
+
+	/*
+	 * Open code untransformRelOptions(), it seems unnecessary to do the full
+	 * transformation here, as that would introduce more memory allocations.
+	 * The code ends up being similarly long as not open-coding.
+	 */
+
+	deconstruct_array_builtin(DatumGetArrayTypeP(attoptions), TEXTOID,
+							  &optiondatums, NULL, &noptions);
+
+	for (int i = 0; i < noptions; i++)
+	{
+		char	   *optname;
+		char	   *optval;
+
+		optname = TextDatumGetCString(optiondatums[i]);
+		optval = strchr(optname, '=');
+
+		if (optval)
+		{
+			*optval++ = '\0';
+
+			if (strcmp(optname, ATTRIBUTE_VERSION_ATTOPTION) == 0)
+			{
+				char	   *endptr;
+
+				errno = 0;
+				version = strtou64(optval, &endptr, 10);
+
+				/*
+				 * Don't accept -, any range errors, require whole string to
+				 * be consumed. This should never be reached, but we shouldn't
+				 * just ignore errors.
+				 */
+				if (!isdigit((unsigned char) *optval) || endptr == optval ||
+					*endptr != 0 || errno != 0)
+					elog(ERROR, "could not parse attribute version \"%s\"", optval);
+				pfree(optname);
+				break;
+			}
+		}
+		pfree(optname);
+	}
+
+	pfree(optiondatums);
+
+	return version;
+}
+
+/*
+ * Return a new attoptions text[] datum with the attribute version set, or
+ * removed if version is InvalidXLogRecPtr.  Other options already present in
+ * attoptions (if not isnull) are kept; any previous version is replaced.
+ */
+Datum
+attoptions_set_version(Datum attoptions, bool isnull, XLogRecPtr version)
+{
+	DefElem    *def;
+	List	   *defList;
+	String	   *str;
+	bool		isReset = XLogRecPtrIsInvalid(version);
+
+	/*
+	 * Using the full reloptions machinery here isn't the cheapest, but
+	 * compared to the cost of DDL it should not matter.
+	 */
+
+	if (isReset)
+		str = NULL;
+	else
+		str = makeString(psprintf("%llu", (long long unsigned) version));
+
+	def = makeDefElem(ATTRIBUTE_VERSION_ATTOPTION, (Node *) str, -1);
+	defList = list_make1(def);
+
+	return transformRelOptions(isnull ? (Datum) 0 : attoptions,
+							   defList, NULL, NULL, false, isReset);
+}
+
 /*
  * Extract and parse reloptions from a pg_class tuple.
  *
@@ -2421,6 +2521,16 @@ attribute_reloptions(Datum reloptions, bool validate)
 		{"n_distinct_inherited", RELOPT_TYPE_REAL, offsetof(AttributeOpts, n_distinct_inherited)}
 	};
 
+	/*
+	 * The reserved attribute version key is stored inside attoptions in the
+	 * back branches, but it is not a user-visible / settable option.  Strip
+	 * it before parsing so it is neither validated nor rejected as an
+	 * unrecognized option.
+	 */
+	if (DatumGetPointer(reloptions) != NULL)
+		reloptions = attoptions_set_version(reloptions, false,
+											InvalidXLogRecPtr);
+
 	return (bytea *) build_reloptions(reloptions, validate,
 									  RELOPT_KIND_ATTRIBUTE,
 									  sizeof(AttributeOpts),
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`assert_toast_defaults_unsettable`
- 外部函数：`deconstruct_array_builtin`
- 外部函数：`ereport`
- 外部函数：`errcode`
- 外部函数：`errmsg`
- 外部函数：`parse_one_reloption`
- 外部函数：`pfree`
- 外部函数：`strchr`
- 外部函数：`strncmp`
- 大写宏：`ERRCODE_INVALID_PARAMETER_VALUE`
- 大写宏：`ERROR`
- 大写宏：`NULL`
- 大写宏：`PG_TERNARY_UNSET`
- 大写宏：`RELOPT_KIND_HEAP`
- 大写宏：`RELOPT_KIND_TOAST`
- 大写宏：`RELOPT_TYPE_BOOL`
- 大写宏：`RELOPT_TYPE_ENUM`
- 大写宏：`RELOPT_TYPE_INT`
- 大写宏：`RELOPT_TYPE_REAL`
- 大写宏：`RELOPT_TYPE_STRING`
- 大写宏：`RELOPT_TYPE_TERNARY`
- 大写宏：`TEXTOID`
- 大写宏：`TOAST`
- 大写宏：`USE_ASSERT_CHECKING`
- 大写宏：`VARDATA`
- 大写宏：`VARHDRSZ`
- 大写宏：`VARSIZE`
- 外部类型：`ArrayType`
- 外部类型：`Build`
- 外部类型：`Datum`
- 外部类型：`Extract`
- 外部类型：`It`
- 外部类型：`Neither`
- 外部类型：`Search`
- 外部类型：`Ternaries`
- 外部类型：`TopMemoryContext`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-638be3cc84` → 本草稿移入 `cases/defect/auto-postgres-638be3cc84/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
