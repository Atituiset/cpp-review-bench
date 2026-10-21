# auto-postgres-73e1ceb464

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#456ee18045d03f8eb7194f7b7fcf53595704b44b](https://github.com/postgres/postgres/commit/456ee18045d03f8eb7194f7b7fcf53595704b44b) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-21 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 7 |
| 编译错误数（gcc syntax-only） | 3（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #456ee18045d03f8eb7194f7b7fcf53595704b44b (https://github.com/postgres/postgres/commit/456ee18045d03f8eb7194f7b7fcf53595704b44b)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 456ee18045d03f8eb7194f7b7fcf53595704b44b Fix sorting a child rel by an expression containing a PlaceHolderVar :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -6176,7 +6176,7 @@ make_incrementalsort(Plan *lefttree, int numCols, int nPresortedCols,
  * Input parameters:
  *	  'lefttree' is the plan node which yields input tuples
  *	  'pathkeys' is the list of pathkeys by which the result is to be sorted
- *	  'relids' identifies the child relation being sorted, if any
+ *	  'relids' identifies the relation being sorted, or is NULL
  *	  'reqColIdx' is NULL or an array of required sort key column numbers
  *	  'adjust_tlist_in_place' is true if lefttree must be modified in-place
  *
@@ -6188,7 +6188,9 @@ make_incrementalsort(Plan *lefttree, int numCols, int nPresortedCols,
  * When looking for matches to an EquivalenceClass's members, we will only
  * consider child EC members if they belong to given 'relids'.  This protects
  * against possible incorrect matches to child expressions that contain no
- * Vars.
+ * Vars.  When computing a sort expression for a child relation, we will not
+ * use parent EC members either, since their PlaceHolderVars could falsely
+ * match the translated ones in the child's tlist.
  *
  * If reqColIdx isn't NULL then it contains sort key column numbers that
  * we should match.  This is used when making child plans for a MergeAppend;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 大写宏：`NULL`
- 外部类型：`EquivalenceClass`
- 外部类型：`If`
- 外部类型：`Input`
- 外部类型：`MergeAppend`
- 外部类型：`This`
- 外部类型：`Vars`
- 外部类型：`When`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-postgres-73e1ceb464` → 本草稿移入 `cases/defect/auto-postgres-73e1ceb464/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
