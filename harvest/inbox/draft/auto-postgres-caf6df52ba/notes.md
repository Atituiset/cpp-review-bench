# auto-postgres-caf6df52ba

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#388f2db2994e216f15f004c4addbe4851871f954](https://github.com/postgres/postgres/commit/388f2db2994e216f15f004c4addbe4851871f954) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-26 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 31 |
| 编译错误数（gcc syntax-only） | 5（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #388f2db2994e216f15f004c4addbe4851871f954 (https://github.com/postgres/postgres/commit/388f2db2994e216f15f004c4addbe4851871f954)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 673；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 388f2db2994e216f15f004c4addbe4851871f954 pg_upgrade: Don't run the not-null inheritance check on v18 clusters :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -670,7 +670,7 @@ check_and_dump_old_cluster(void)
 	 * parents columns have, but schema restore fails for them.  Verify there
 	 * are none, iff applicable.
 	 */
-	if (GET_MAJOR_VERSION(old_cluster.major_version) <= 1800)
+	if (GET_MAJOR_VERSION(old_cluster.major_version) <= 1700)
 		check_for_not_null_inheritance(&old_cluster);
 
 	/*
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`check_ok`
- 外部函数：`column`
- 外部函数：`fclose`
- 外部函数：`fopen_priv`
- 外部函数：`fprintf`
- 外部函数：`pg_fatal`
- 外部函数：`pg_log`
- 外部函数：`prep_status`
- 外部函数：`snprintf`
- 外部函数：`upgrade_task_add_step`
- 外部函数：`upgrade_task_create`
- 外部函数：`upgrade_task_free`
- 外部函数：`upgrade_task_run`
- 大写宏：`ALTER`
- 大写宏：`AND`
- 大写宏：`FROM`
- 大写宏：`GET_MAJOR_VERSION`
- 大写宏：`NOT`
- 大写宏：`NULL`
- 大写宏：`PG_REPORT`
- 大写宏：`SELECT`
- 大写宏：`SET`
- 大写宏：`TABLE`
- 大写宏：`WHERE`
- 外部类型：`Checking`
- 外部类型：`ClusterInfo`
- 外部类型：`DbInfo`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`PGresult`
- 外部类型：`UpgradeTask`
- 外部类型：`UpgradeTaskReport`
- 外部类型：`Verify`
- 外部类型：`You`
- 外部类型：`Your`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-caf6df52ba` → 本草稿移入 `cases/defect/auto-postgres-caf6df52ba/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
