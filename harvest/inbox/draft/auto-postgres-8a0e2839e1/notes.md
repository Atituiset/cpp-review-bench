# auto-postgres-8a0e2839e1

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#8e27c9261c9dd6127e16ced998d37ed7a8cb2b7d](https://github.com/postgres/postgres/commit/8e27c9261c9dd6127e16ced998d37ed7a8cb2b7d) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-13 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 33 |
| 编译错误数（gcc syntax-only） | 16（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #8e27c9261c9dd6127e16ced998d37ed7a8cb2b7d (https://github.com/postgres/postgres/commit/8e27c9261c9dd6127e16ced998d37ed7a8cb2b7d)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 185；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 8e27c9261c9dd6127e16ced998d37ed7a8cb2b7d Fix assorted confusion about parameter names. :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -182,8 +182,8 @@ typedef struct
 	List	   *index_tlist;	/* referent for INDEX_VAR Vars */
 	/* Special namespace representing a function signature: */
 	char	   *funcname;
-	int			numargs;
-	char	  **argnames;
+	int			numargs;		/* number of input arguments */
+	char	  **argnames;		/* names of input args (NULL if unnamed) */
 } deparse_namespace;
 
 /*
@@ -3561,19 +3561,29 @@ pg_get_function_arg_default(PG_FUNCTION_ARGS)
 static void
 print_function_sqlbody(StringInfo buf, HeapTuple proctup)
 {
-	int			numargs;
-	Oid		   *argtypes;
-	char	  **argnames;
-	char	   *argmodes;
+	Datum		proargnames;
+	Datum		proargmodes;
+	bool		isNull;
 	deparse_namespace dpns = {0};
 	Datum		tmp;
 	Node	   *n;
 
 	dpns.funcname = pstrdup(NameStr(((Form_pg_proc) GETSTRUCT(proctup))->proname));
-	numargs = get_func_arg_info(proctup,
-								&argtypes, &argnames, &argmodes);
-	dpns.numargs = numargs;
-	dpns.argnames = argnames;
+
+	proargnames = SysCacheGetAttr(PROCOID, proctup,
+								  Anum_pg_proc_proargnames,
+								  &isNull);
+	if (isNull)
+		proargnames = PointerGetDatum(NULL);	/* just to be sure */
+
+	proargmodes = SysCacheGetAttr(PROCOID, proctup,
+								  Anum_pg_proc_proargmodes,
+								  &isNull);
+	if (isNull)
+		proargmodes = PointerGetDatum(NULL);	/* just to be sure */
+
+	dpns.numargs = get_func_input_arg_names(proargnames, proargmodes,
+											&dpns.argnames);
 
 	tmp = SysCacheGetAttrNotNull(PROCOID, proctup, Anum_pg_proc_prosqlbody);
 	n = stringToNode(TextDatumGetCString(tmp));
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`get_func_arg_info`
- 外部函数：`pstrdup`
- 外部函数：`stringToNode`
- 大写宏：`GETSTRUCT`
- 大写宏：`INDEX_VAR`
- 大写宏：`INNER_VAR`
- 大写宏：`NEW`
- 大写宏：`NULL`
- 大写宏：`OLD`
- 大写宏：`OUTER_VAR`
- 大写宏：`PROCOID`
- 大写宏：`RETURNING`
- 大写宏：`USING`
- 外部类型：`Anum_pg_proc_prosqlbody`
- 外部类型：`AppendRelInfo`
- 外部类型：`Are`
- 外部类型：`Array`
- 外部类型：`CommonTableExpr`
- 外部类型：`Datum`
- 外部类型：`Form_pg_proc`
- 外部类型：`HeapTuple`
- 外部类型：`List`
- 外部类型：`Node`
- 外部类型：`Oid`
- 外部类型：`Parallel`
- 外部类型：`Plan`
- 外部类型：`RTEs`
- 外部类型：`RangeTblEntry`
- 外部类型：`Remaining`
- 外部类型：`Special`
- 外部类型：`StringInfo`
- 外部类型：`SubPlans`
- 外部类型：`Vars`
- 外部类型：`Workspace`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-8a0e2839e1` → 本草稿移入 `cases/defect/auto-postgres-8a0e2839e1/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
