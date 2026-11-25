# auto-postgres-b70a3f76c0

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#2bdbde8b3bd782ec18168e19b70ccaa945ab0177](https://github.com/postgres/postgres/commit/2bdbde8b3bd782ec18168e19b70ccaa945ab0177) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-25 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 24 |
| 编译错误数（gcc syntax-only） | 3（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #2bdbde8b3bd782ec18168e19b70ccaa945ab0177 (https://github.com/postgres/postgres/commit/2bdbde8b3bd782ec18168e19b70ccaa945ab0177)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 10247；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 2bdbde8b3bd782ec18168e19b70ccaa945ab0177 Adjust an error message to emphasize the inheritance relationship :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -10244,7 +10244,7 @@ ATPrepAddPrimaryKey(AlteredTableInfo *tab, Relation rel, AlterTableCmd *cmd,
 				if (!tup)
 					ereport(ERROR,
 							errcode(ERRCODE_INVALID_TABLE_DEFINITION),
-							errmsg("column \"%s\" of table \"%s\" is not marked NOT NULL",
+							errmsg("column \"%s\" in child table \"%s\" is not marked NOT NULL",
 								   strVal(column), get_rel_name(childrelid)));
 				/* verify it's good enough */
 				verifyNotNullPKCompatible(tup, strVal(column));
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`elog`
- 外部函数：`ereport`
- 外部函数：`errcode`
- 外部函数：`errdetail`
- 外部函数：`errhint`
- 外部函数：`errmsg`
- 外部函数：`get_rel_name`
- 外部函数：`strVal`
- 大写宏：`ALTER`
- 大写宏：`CONSTRAINT`
- 大写宏：`CONSTRAINT_NOTNULL`
- 大写宏：`ERRCODE_INVALID_TABLE_DEFINITION`
- 大写宏：`ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE`
- 大写宏：`ERROR`
- 大写宏：`GETSTRUCT`
- 大写宏：`INHERIT`
- 大写宏：`NOT`
- 大写宏：`NULL`
- 大写宏：`TABLE`
- 大写宏：`VALID`
- 大写宏：`VALIDATE`
- 外部类型：`Form_pg_constraint`
- 外部类型：`HeapTuple`
- 外部类型：`The`
- 外部类型：`You`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-b70a3f76c0` → 本草稿移入 `cases/defect/auto-postgres-b70a3f76c0/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
