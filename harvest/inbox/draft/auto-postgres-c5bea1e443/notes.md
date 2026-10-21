# auto-postgres-c5bea1e443

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
- 候选初判锚点行: 4（原始 PR diff 行 1029；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 456ee18045d03f8eb7194f7b7fcf53595704b44b Fix sorting a child rel by an expression containing a PlaceHolderVar :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -982,7 +982,8 @@ find_ec_member_matching_expr(EquivalenceClass *ec,
  * compute an expression in this way, setrefs.c is going to insist on exact
  * matches of Vars to the source tlist.
  *
- * Child EC members are ignored unless they belong to given 'relids'.
+ * If 'relids' is given, EC members are ignored unless they belong to it.  If
+ * 'relids' is NULL, all child members are ignored.
  * Also, non-parallel-safe expressions are ignored if 'require_parallel_safe'.
  *
  * Note: some callers pass root == NULL for notational reasons.  This is OK
@@ -1024,9 +1025,13 @@ find_computable_ec_member(PlannerInfo *root,
 			continue;
 
 		/*
-		 * Ignore child members unless they belong to the requested rel.
+		 * Ignore child members unless they belong to the requested rel.  If
+		 * 'relids' is given, ignore parent members that don't belong to it
+		 * either: for a child rel, a parent member's PlaceHolderVars would
+		 * otherwise match the translated ones in "exprs", since equal()
+		 * ignores phexpr and phrels when comparing PlaceHolderVars.
 		 */
-		if (em->em_is_child &&
+		if ((em->em_is_child || relids != NULL) &&
 			!bms_is_subset(em->em_relids, relids))
 			continue;
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`bms_is_subset`
- 大写宏：`NULL`
- 外部类型：`Also`
- 外部类型：`Child`
- 外部类型：`Ignore`
- 外部类型：`Note`
- 外部类型：`This`
- 外部类型：`Vars`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-c5bea1e443` → 本草稿移入 `cases/defect/auto-postgres-c5bea1e443/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
