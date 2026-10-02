# auto-postgres-76316956d8

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#42e96cf2fe095c822f76bc94669ea875cb1e3351](https://github.com/postgres/postgres/commit/42e96cf2fe095c822f76bc94669ea875cb1e3351) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-02 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 2 |
| 编译错误数（gcc syntax-only） | 4（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #42e96cf2fe095c822f76bc94669ea875cb1e3351 (https://github.com/postgres/postgres/commit/42e96cf2fe095c822f76bc94669ea875cb1e3351)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 42e96cf2fe095c822f76bc94669ea875cb1e3351 Refresh autovacuum costs while waiting for parallel workers. :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1814,8 +1814,33 @@ autovac_recalculate_workers_for_balance(void)
 	}
 
 	if (nworkers_for_balance != orig_nworkers_for_balance)
+	{
 		pg_atomic_write_u32(&AutoVacuumShmem->av_nworkersForBalance,
 							nworkers_for_balance);
+
+		/*
+		 * Wake up the autovacuum workers sharing the cost limit so that they
+		 * pick up the new count. An autovacuum worker that is vacuuming does
+		 * that on its next nap anyway, but one running a parallel vacuum
+		 * (leader) that is only waiting for its parallel workers to finish
+		 * never naps, and nothing else would tell it.
+		 *
+		 * Only the waiting leaders need this, but knowing which ones are
+		 * waiting would need more state. For an autovacuum worker that is not
+		 * in a latch wait, SetLatch() sends no signal and only marks the
+		 * latch set, which costs one early return from its next latch wait.
+		 */
+		dlist_foreach(iter, &AutoVacuumShmem->av_runningWorkers)
+		{
+			WorkerInfo	worker = dlist_container(WorkerInfoData, wi_links, iter.cur);
+
+			if (worker->wi_proc == NULL ||
+				pg_atomic_unlocked_test_flag(&worker->wi_dobalance))
+				continue;
+
+			SetLatch(&worker->wi_proc->procLatch);
+		}
+	}
 }
 
 /*
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`pg_atomic_write_u32`
- 外部类型：`AutoVacuumShmem`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-76316956d8` → 本草稿移入 `cases/defect/auto-postgres-76316956d8/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
