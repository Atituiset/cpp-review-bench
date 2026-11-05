# auto-postgres-b6864a6bd1

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#fc1900915b140aaed33fdcccdf85eec1a0035755](https://github.com/postgres/postgres/commit/fc1900915b140aaed33fdcccdf85eec1a0035755) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-05 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 7 |
| 编译错误数（gcc syntax-only） | 7（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #fc1900915b140aaed33fdcccdf85eec1a0035755 (https://github.com/postgres/postgres/commit/fc1900915b140aaed33fdcccdf85eec1a0035755)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 1967；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT fc1900915b140aaed33fdcccdf85eec1a0035755 Avoid resetting subscription two_phase state to pending when already enabled. :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1963,12 +1963,19 @@ AlterSubscription(ParseState *pstate, AlterSubscriptionStmt *stmt,
 								 errmsg("cannot disable \"two_phase\" when prepared transactions exist"),
 								 errhint("Resolve these transactions and try again.")));
 
-					/* Change system catalog accordingly */
-					values[Anum_pg_subscription_subtwophasestate - 1] =
-						CharGetDatum(opts.twophase ?
-									 LOGICALREP_TWOPHASE_STATE_PENDING :
-									 LOGICALREP_TWOPHASE_STATE_DISABLED);
-					replaces[Anum_pg_subscription_subtwophasestate - 1] = true;
+					/*
+					 * Change system catalog accordingly, but do not reset the
+					 * state to PENDING if two_phase is already enabled.
+					 */
+					if (!(opts.twophase &&
+						  sub->twophasestate == LOGICALREP_TWOPHASE_STATE_ENABLED))
+					{
+						values[Anum_pg_subscription_subtwophasestate - 1] =
+							CharGetDatum(opts.twophase ?
+										 LOGICALREP_TWOPHASE_STATE_PENDING :
+										 LOGICALREP_TWOPHASE_STATE_DISABLED);
+						replaces[Anum_pg_subscription_subtwophasestate - 1] = true;
+					}
 				}
 
 				if (IsSet(opts.specified_opts, SUBOPT_FAILOVER))
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`errhint`
- 外部函数：`errmsg`
- 大写宏：`LOGICALREP_TWOPHASE_STATE_DISABLED`
- 大写宏：`LOGICALREP_TWOPHASE_STATE_PENDING`
- 外部类型：`Anum_pg_subscription_subtwophasestate`
- 外部类型：`Change`
- 外部类型：`Resolve`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-b6864a6bd1` → 本草稿移入 `cases/defect/auto-postgres-b6864a6bd1/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
