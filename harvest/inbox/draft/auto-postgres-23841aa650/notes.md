# auto-postgres-23841aa650

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#7d47e41238004b6b8bce076d4bb76d858257b58d](https://github.com/postgres/postgres/commit/7d47e41238004b6b8bce076d4bb76d858257b58d) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-09-27 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 3 |
| 编译错误数（gcc syntax-only） | 3（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #7d47e41238004b6b8bce076d4bb76d858257b58d (https://github.com/postgres/postgres/commit/7d47e41238004b6b8bce076d4bb76d858257b58d)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 7d47e41238004b6b8bce076d4bb76d858257b58d Fix privilege handling in postgres_fdw statistics import. :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -5832,11 +5832,10 @@ fetch_remote_statistics(Relation relation,
 	}
 
 	/*
-	 * Get connection to the foreign server.  Connection manager will
-	 * establish new connection if necessary.
-	 *
-	 * Note that unlike the sampling case, we only query pg_class and
-	 * pg_stats, so we do the remote access as the current user.
+	 * Get the connection to use.  We do the remote access as the table's
+	 * owner.  Note that unlike AnalyzeForeignTable(), the core code would
+	 * already have switched us to the table's owner, before we are called
+	 * from ImportForeignStatistics().
 	 */
 	user = GetUserMapping(GetUserId(), table->serverid);
 	conn = GetConnection(user, false, NULL);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 大写宏：`NULL`
- 外部类型：`Connection`
- 外部类型：`Get`
- 外部类型：`Note`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-23841aa650` → 本草稿移入 `cases/defect/auto-postgres-23841aa650/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
