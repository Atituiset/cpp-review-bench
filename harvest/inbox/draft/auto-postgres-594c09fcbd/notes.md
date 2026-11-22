# auto-postgres-594c09fcbd

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#9b5d4e3331753b3399e84965093109cac8f364bf](https://github.com/postgres/postgres/commit/9b5d4e3331753b3399e84965093109cac8f364bf) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-22 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 18 |
| 编译错误数（gcc syntax-only） | 16（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #9b5d4e3331753b3399e84965093109cac8f364bf (https://github.com/postgres/postgres/commit/9b5d4e3331753b3399e84965093109cac8f364bf)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 866；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 9b5d4e3331753b3399e84965093109cac8f364bf Don't pfree a Path that belongs to a different parent relation. :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -439,7 +439,7 @@ set_cheapest(RelOptInfo *parent_rel)
  *	  memory consumption.  We dare not try to free the substructure of a Path,
  *	  since much of it may be shared with other Paths or the query tree itself;
  *	  but just recycling discarded Path nodes is a very useful savings in
- *	  a large join tree.  We can recycle the List nodes of pathlist, too.
+ *	  a large join tree.
  *
  *	  As noted in optimizer/README, deleting a previously-accepted Path is
  *	  safe because we know that Paths of this rel cannot yet be referenced
@@ -450,6 +450,16 @@ set_cheapest(RelOptInfo *parent_rel)
  *	  referenced as children of BitmapHeapPaths as well as being paths in
  *	  their own right.  Hence, we don't pfree IndexPaths when rejecting them.
  *
+ *	  A related but distinct hazard is a Path that was built for, and is
+ *	  still linked to, some other rel than parent_rel.  Some callers
+ *	  (e.g. create_ordered_paths()) pass in another relation's Path without
+ *	  copying it, so that it remains a live member of its original parent's
+ *	  own pathlist.  We detect this generically by comparing the Path's
+ *	  path->parent against parent_rel: they will be equal if the Path was
+ *	  actually built for this rel.  If they are not, we do not free the Path
+ *	  for fear that a live link to it remains in the original parent rel. (In
+ *	  most cases that link would not be used again, but there are exceptions.)
+ *
  * 'parent_rel' is the relation entry to which the path corresponds.
  * 'new_path' is a potential path for parent_rel.
  *
@@ -623,11 +633,9 @@ add_path(RelOptInfo *parent_rel, Path *new_path)
 		{
 			parent_rel->pathlist = foreach_delete_current(parent_rel->pathlist,
 														  p1);
-
-			/*
-			 * Delete the data pointed-to by the deleted cell, if possible
-			 */
-			if (!IsA(old_path, IndexPath))
+			/* Delete the old path, if it's safe; see header comment */
+			if (!IsA(old_path, IndexPath) &&
+				old_path->parent == parent_rel)
 				pfree(old_path);
 		}
 		else
@@ -659,8 +667,9 @@ add_path(RelOptInfo *parent_rel, Path *new_path)
 	}
 	else
 	{
-		/* Reject and recycle the new path */
-		if (!IsA(new_path, IndexPath))
+		/* Reject and recycle the new path, if it's safe */
+		if (!IsA(new_path, IndexPath) &&
+			new_path->parent == parent_rel)
 			pfree(new_path);
 	}
 }
@@ -786,8 +795,18 @@ add_path_precheck(RelOptInfo *parent_rel, int disabled_nodes,
  *	  another partial path; this requires that there be no other references to
  *	  such paths yet.  Hence, GatherPaths must not be created for a rel until
  *	  we're done creating all partial paths for it.  Unlike add_path, we don't
- *	  take an exception for IndexPaths as partial index paths won't be
+ *	  need an exception for IndexPaths as partial index paths won't be
  *	  referenced by partial BitmapHeapPaths.
+ *
+ *	  But, like add_path, we refrain from pfreeing partial paths that belong
+ *	  to some other rel (i.e. path->parent != parent_rel).  Ordinarily every
+ *	  partial path submitted here was freshly built for parent_rel, but
+ *	  grouping_planner intentionally passes a scan/join rel's paths and
+ *	  partial paths to add_path and add_partial_path for the query's final_rel
+ *	  (the latter so that Gathers can be built from them in outer query
+ *	  levels).  In that usage there are edge cases where add_path can keep a
+ *	  Gather while add_partial_path will reject its underlying partial path,
+ *	  so that if we pfree the partial path we'll have a broken Gather path.
  */
 void
 add_partial_path(RelOptInfo *parent_rel, Path *new_path)
@@ -863,7 +882,9 @@ add_partial_path(RelOptInfo *parent_rel, Path *new_path)
 		{
 			parent_rel->partial_pathlist =
 				foreach_delete_current(parent_rel->partial_pathlist, p1);
-			pfree(old_path);
+			/* Delete the old path, if it's safe; see header comment */
+			if (old_path->parent == parent_rel)
+				pfree(old_path);
 		}
 		else
 		{
@@ -894,8 +915,9 @@ add_partial_path(RelOptInfo *parent_rel, P
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`add_partial_path`
- 外部函数：`foreach_delete_current`
- 外部函数：`pfree`
- 大写宏：`README`
- 外部类型：`As`
- 外部类型：`BitmapHeapPaths`
- 外部类型：`Delete`
- 外部类型：`GatherPaths`
- 外部类型：`Hence`
- 外部类型：`IndexPath`
- 外部类型：`IndexPaths`
- 外部类型：`List`
- 外部类型：`Path`
- 外部类型：`Paths`
- 外部类型：`Reject`
- 外部类型：`RelOptInfo`
- 外部类型：`Unlike`
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

1. 完成上面检查清单后评论 `/case accept auto-postgres-594c09fcbd` → 本草稿移入 `cases/defect/auto-postgres-594c09fcbd/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
