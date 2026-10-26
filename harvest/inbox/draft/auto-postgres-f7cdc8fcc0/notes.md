# auto-postgres-f7cdc8fcc0

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#f9bc96514afc9c71d533568f62dc613be8356c4d](https://github.com/postgres/postgres/commit/f9bc96514afc9c71d533568f62dc613be8356c4d) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-26 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 25 |
| 编译错误数（gcc syntax-only） | 3（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #f9bc96514afc9c71d533568f62dc613be8356c4d (https://github.com/postgres/postgres/commit/f9bc96514afc9c71d533568f62dc613be8356c4d)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 2238；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT f9bc96514afc9c71d533568f62dc613be8356c4d Look through wrapper paths when collecting enforced clause serials. :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2152,6 +2152,27 @@ get_appendrel_parampathinfo(RelOptInfo *appendrel, Relids required_outer)
 	return ppi;
 }
 
+/*
+ * get_wrapper_parampathinfo
+ *		Get the ParamPathInfo for a parameterized wrapper path.
+ *
+ * If a wrapper-type path, such as a MaterialPath, might be parameterized,
+ * use this function to compute its param_info.  This is suitable only when
+ * the wrapper path cannot add or remove any parameterization from its
+ * subpath, which typically means that it evaluates no quals nor tlist
+ * expressions.
+ */
+ParamPathInfo *
+get_wrapper_parampathinfo(Path *subpath)
+{
+	/*
+	 * Currently we just re-use the subpath's ParamPathInfo, if any.  The
+	 * ppi_req_outer value is correct given the assumption stated above, and
+	 * none of the other fields are required to be valid for a wrapper path.
+	 */
+	return subpath->param_info;
+}
+
 /*
  * Returns a ParamPathInfo for the parameterization given by required_outer, if
  * already available in the given rel. Returns NULL otherwise.
@@ -2176,79 +2197,123 @@ find_param_path_info(RelOptInfo *rel, Relids required_outer)
  * get_param_path_clause_serials
  *		Given a parameterized Path, return the set of pushed-down clauses
  *		(identified by rinfo_serial numbers) enforced within the Path.
+ *
+ * Callers should use this in preference to fetching ppi_serials themselves.
  */
 Bitmapset *
 get_param_path_clause_serials(Path *path)
 {
 	if (path->param_info == NULL)
 		return NULL;			/* not parameterized */
 
-	/*
-	 * We don't currently support parameterized MergeAppend paths, as
-	 * explained in the comments for generate_orderedappend_paths.
-	 */
-	Assert(!IsA(path, MergeAppendPath));
-
-	if (IsA(path, NestPath) ||
-		IsA(path, MergePath) ||
-		IsA(path, HashPath))
+	switch (nodeTag(path))
 	{
-		/*
-		 * For a join path, combine clauses enforced within either input path
-		 * with those enforced as joinrestrictinfo in this path.  Note that
-		 * joinrestrictinfo may include some non-pushed-down clauses, but for
-		 * current purposes it's okay if we include those in the result. (To
-		 * be more careful, we could check for clause_relids overlapping the
-		 * path parameterization, but it's not worth the cycles for now.)
-		 */
-		JoinPath   *jpath = (JoinPath *) path;
-		Bitmapset  *pserials;
-		ListCell   *lc;
+		case T_NestPath:
+		case T_MergePath:
+		case T_HashPath:
+			{
+				/*
+				 * For a join path, combine clauses enforced within either
+				 * input path with those enforced as joinrestrictinfo in this
+				 * path.  Note that joinrestrictinfo may include some
+				 * non-pushed-down clauses, but for current purposes it's okay
+				 * if we include those in the result.  (To be more careful, we
+				 * could check for clause_relids overlapping the path
+				 * parameterization, but it's not worth the cycles for now.)
+				 */
+				JoinPath   *jpath = (JoinPath *) path;
+				Bitmapset  *pserials;
+				ListCell   *lc;
+
+				pserials = NULL;
+				pserials = bms_add_members(pserials,
+										   get_param_path_clause_serials(jpath->outerjoinpath));
+				pserials = bms_add_members(pserials,
+										   get_param_path_clause_serials(jpath->innerjoinpath));
+				foreach(lc, jpath->joinrestrictinfo)
+				{
+					RestrictInfo *rinfo = (RestrictInfo *) lfirst(lc);
 
-		pserials = NULL;
-		pserials = bms_add_members(pserials,
-								   get_param_path_clause_serials(jpath->outerjoinpath));
-		pserials = bms_add_members(pserials,
-								   get_param_path_clause_serials(jpath->innerjoinpath));
-		foreach(lc, jpath->joinrestrictinfo)
-		{
-			RestrictInfo *rinfo = (RestrictInfo *) lfirst(lc);
+					pserials = bms_add_member(pserials, rinfo->rinfo_serial);
+				}
+				return pserials;
+			}
+		case T_AppendPath:
+			{
+				/*
+				 * For an appendrel, take the intersection of the sets of
+				 * clauses enforced in each input path.
+				 */
+				AppendPath *apath = (AppendPath *) path;
+				Bitmapset  *pserials;
+				ListCell   *lc;
 
-			pserials = bms_add_m
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`bms_add_member`
- 外部函数：`bms_add_members`
- 外部函数：`bms_copy`
- 外部函数：`bms_int_members`
- 外部函数：`lfirst`
- 外部函数：`list_head`
- 大写宏：`NULL`
- 外部类型：`AppendPath`
- 外部类型：`Bitmapset`
- 外部类型：`For`
- 外部类型：`Given`
- 外部类型：`HashPath`
- 外部类型：`JoinPath`
- 外部类型：`ListCell`
- 外部类型：`MergeAppend`
- 外部类型：`MergeAppendPath`
- 外部类型：`MergePath`
- 外部类型：`NestPath`
- 外部类型：`Note`
- 外部类型：`Otherwise`
- 外部类型：`ParamPathInfo`
- 外部类型：`Path`
- 外部类型：`RestrictInfo`
- 外部类型：`Returns`
- 外部类型：`To`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-f7cdc8fcc0` → 本草稿移入 `cases/defect/auto-postgres-f7cdc8fcc0/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
