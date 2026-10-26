# auto-postgres-8f0220bb69

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
| 外部依赖数（dep_count） | 13 |
| 编译错误数（gcc syntax-only） | 46（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #f9bc96514afc9c71d533568f62dc613be8356c4d (https://github.com/postgres/postgres/commit/f9bc96514afc9c71d533568f62dc613be8356c4d)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 1723；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT f9bc96514afc9c71d533568f62dc613be8356c4d Look through wrapper paths when collecting enforced clause serials. :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1371,7 +1371,9 @@ create_append_path(PlannerInfo *root,
 	 * to apply get_baserel_parampathinfo to construct a full ParamPathInfo
 	 * for the path.  This supports building a Memoize path atop this path,
 	 * and if this is a partitioned table the info may be useful for run-time
-	 * pruning (cf make_partition_pruneinfo()).
+	 * pruning (cf make_partition_pruneinfo()).  Note that its ppi_clauses and
+	 * ppi_serials may include clauses that the child paths cannot enforce;
+	 * see comments for ParamPathInfo.
 	 *
 	 * However, if we don't have "root" then that won't work and we fall back
 	 * on the simpler get_appendrel_parampathinfo.  There's no point in doing
@@ -1720,7 +1722,7 @@ create_material_path(RelOptInfo *rel, Path *subpath, bool enabled)
 	pathnode->path.pathtype = T_Material;
 	pathnode->path.parent = rel;
 	pathnode->path.pathtarget = rel->reltarget;
-	pathnode->path.param_info = subpath->param_info;
+	pathnode->path.param_info = get_wrapper_parampathinfo(subpath);
 	pathnode->path.parallel_aware = false;
 	pathnode->path.parallel_safe = rel->consider_parallel &&
 		subpath->parallel_safe;
@@ -1756,7 +1758,7 @@ create_memoize_path(PlannerInfo *root, RelOptInfo *rel, Path *subpath,
 	pathnode->path.pathtype = T_Memoize;
 	pathnode->path.parent = rel;
 	pathnode->path.pathtarget = rel->reltarget;
-	pathnode->path.param_info = subpath->param_info;
+	pathnode->path.param_info = get_wrapper_parampathinfo(subpath);
 	pathnode->path.parallel_aware = false;
 	pathnode->path.parallel_safe = rel->consider_parallel &&
 		subpath->parallel_safe;
@@ -2613,7 +2615,7 @@ create_projection_path(PlannerInfo *root,
 	pathnode->path.pathtype = T_Result;
 	pathnode->path.parent = rel;
 	pathnode->path.pathtarget = target;
-	pathnode->path.param_info = subpath->param_info;
+	pathnode->path.param_info = get_wrapper_parampathinfo(subpath);
 	pathnode->path.parallel_aware = false;
 	pathnode->path.parallel_safe = rel->consider_parallel &&
 		subpath->parallel_safe &&
@@ -2868,7 +2870,7 @@ create_incremental_sort_path(PlannerInfo *root,
 	pathnode->path.parent = rel;
 	/* Sort doesn't project, so use source path's pathtarget */
 	pathnode->path.pathtarget = subpath->pathtarget;
-	pathnode->path.param_info = subpath->param_info;
+	pathnode->path.param_info = get_wrapper_parampathinfo(subpath);
 	pathnode->path.parallel_aware = false;
 	pathnode->path.parallel_safe = rel->consider_parallel &&
 		subpath->parallel_safe;
@@ -2916,7 +2918,7 @@ create_sort_path(PlannerInfo *root,
 	pathnode->path.parent = rel;
 	/* Sort doesn't project, so use source path's pathtarget */
 	pathnode->path.pathtarget = subpath->pathtarget;
-	pathnode->path.param_info = subpath->param_info;
+	pathnode->path.param_info = get_wrapper_parampathinfo(subpath);
 	pathnode->path.parallel_aware = false;
 	pathnode->path.parallel_safe = rel->consider_parallel &&
 		subpath->parallel_safe;
@@ -3017,7 +3019,7 @@ create_unique_path(PlannerInfo *root,
 	pathnode->path.parent = rel;
 	/* Unique doesn't project, so use source path's pathtarget */
 	pathnode->path.pathtarget = subpath->pathtarget;
-	pathnode->path.param_info = subpath->param_info;
+	pathnode->path.param_info = get_wrapper_parampathinfo(subpath);
 	pathnode->path.parallel_aware = false;
 	pathnode->path.parallel_safe = rel->consider_parallel &&
 		subpath->parallel_safe;
@@ -3083,7 +3085,7 @@ create_agg_path(PlannerInfo *root,
 	pathnode->path.pathtype = T_Agg;
 	pathnode->path.parent = rel;
 	pathnode->path.pathtarget = target;
-	pathnode->path.param_info = subpath->param_info;
+	pathnode->path.param_info = get_wrapper_parampathinfo(subpath);
 	pathnode->path.parallel_aware = false;
 	pathnode->path.parallel_safe = rel->consider_parallel &&
 		subpath->parallel_safe;
@@ -3167,7 +3169,7 @@ create_groupingsets_path(PlannerInfo *root,
 	pathnode->path.pathtype = T_Agg;
 	pathnode->path.parent = rel;
 	pathnode->path.pathtarget = target;
-	pathnode->path.param_info = subpath->param_info;
+	pathnode->path.param_in
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`make_partition_pruneinfo`
- 外部函数：`pruning`
- 外部类型：`However`
- 外部类型：`Memoize`
- 外部类型：`ParamPathInfo`
- 外部类型：`Sort`
- 外部类型：`T_Agg`
- 外部类型：`T_Material`
- 外部类型：`T_Memoize`
- 外部类型：`T_Result`
- 外部类型：`There`
- 外部类型：`This`
- 外部类型：`Unique`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-8f0220bb69` → 本草稿移入 `cases/defect/auto-postgres-8f0220bb69/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
