# auto-linux-512c8315bf

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#88328f850d956d267d11e2ee225ee1eaa902b8e0](https://github.com/torvalds/linux/commit/88328f850d956d267d11e2ee225ee1eaa902b8e0) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-19 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 28 |
| 编译错误数（gcc syntax-only） | 27（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #88328f850d956d267d11e2ee225ee1eaa902b8e0 (https://github.com/torvalds/linux/commit/88328f850d956d267d11e2ee225ee1eaa902b8e0)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 11337；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 88328f850d956d267d11e2ee225ee1eaa902b8e0 Merge tag 'sched-urgent-2026-10-18' of git://git.kernel.org/pub/scm/linux/kernel/git/tip/tip :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -11327,17 +11327,16 @@ void sched_change_end(struct sched_change_ctx *ctx)
 		if (p->sched_class->switched_to)
 			p->sched_class->switched_to(rq, p);
 
+		/*
+		 * Check if the task can preempt the current in case the class
+		 * was upgraded. Note that none of the switch*_from() methods
+		 * know the new class and none of the switch*_to() methods know
+		 * the old class so we must resched here.
+		 */
+		if (ctx->queued)
+			wakeup_preempt(rq, p, 0);
+
 		if (ctx->running) {
-			/*
-			 * If this was a class promotion; let the old class
-			 * know it got preempted. Note that none of the
-			 * switch*_from() methods know the new class and none
-			 * of the switch*_to() methods know the old class.
-			 */
-			if (sched_class_above(p->sched_class, ctx->class)) {
-				rq->next_class->wakeup_preempt(rq, p, 0);
-				rq->next_class = p->sched_class;
-			}
 			/*
 			 * If this was a degradation in class; make sure to
 			 * reschedule.
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`_to`
- 外部函数：`cpu_of`
- 外部函数：`fetch_or`
- 外部函数：`is_idle_task`
- 外部函数：`lockdep_assert_rq_held`
- 外部函数：`rq_clock_skip_update`
- 外部函数：`sched_class_above`
- 外部函数：`set_preempt_need_resched`
- 外部函数：`set_ti_thread_flag`
- 外部函数：`smp_processor_id`
- 外部函数：`smp_send_reschedule`
- 外部函数：`switched_to`
- 外部函数：`task_on_rq_queued`
- 外部函数：`task_thread_info`
- 外部函数：`test_tsk_need_resched`
- 外部函数：`trace_sched_set_need_resched_tp`
- 外部函数：`trace_sched_wake_idle_without_ipi`
- 外部函数：`try_cmpxchg`
- 外部函数：`typeof`
- 大写宏：`TIF_NEED_RESCHED`
- 大写宏：`TIF_NEED_RESCHED_LAZY`
- 外部类型：`Always`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Note`
- 外部类型：`rq`
- 外部类型：`task_struct`
- 外部类型：`thread_info`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-512c8315bf` → 本草稿移入 `cases/defect/auto-linux-512c8315bf/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
