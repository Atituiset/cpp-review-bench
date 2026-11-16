# auto-linux-691cbef772

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#a9daf327f3ff90625204876981f88cfe2b6f6fff](https://github.com/torvalds/linux/commit/a9daf327f3ff90625204876981f88cfe2b6f6fff) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-16 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 28 |
| 编译错误数（gcc syntax-only） | 22（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #a9daf327f3ff90625204876981f88cfe2b6f6fff (https://github.com/torvalds/linux/commit/a9daf327f3ff90625204876981f88cfe2b6f6fff)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 119；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT a9daf327f3ff90625204876981f88cfe2b6f6fff Merge tag 'sched_ext-for-7.4-rc2-fixes' of git://git.kernel.org/pub/scm/linux/kernel/git/tj/sched_ext :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -18,9 +18,32 @@ enum scx_dsp_verdict {
 	SCX_DSP_NONE,		/* nothing to run */
 	SCX_DSP_LOCAL,		/* local DSQ has tasks */
 	SCX_DSP_PREV,		/* keep running @prev */
-	SCX_DSP_RETRY,		/* pick helpers only: restart the pick */
+	SCX_DSP_RETRY,		/* restart the pick */
 };
 
+/**
+ * scx_dsp_prev_stale - Test whether dispatch's @prev went stale
+ * @rq: rq being dispatched for
+ * @prev: @rq's donor when dispatch started
+ *
+ * @prev can stop being @rq's donor while dispatch has the rq lock dropped. With
+ * proxy execution, @prev may not be the running task, and another CPU can reset
+ * the donor to rq->curr. With core scheduling, the core pick runs dispatch for
+ * the sibling rqs too, and a sibling can switch @prev out. Dispatch restarts
+ * the pick when that happened and doesn't hand the old donor to the BPF
+ * scheduler.
+ *
+ * With proxy execution alone, the donor only ever changes to rq->curr during a
+ * pick. Only __schedule() on this CPU sets a different donor, after the pick,
+ * so the restarted pick takes rq->curr as @prev and can't go stale again. Under
+ * core scheduling, the restart goes through the core pick, which already
+ * retries whenever dispatch dropped the lock.
+ */
+static inline bool scx_dsp_prev_stale(struct rq *rq, struct task_struct *prev)
+{
+	return (sched_proxy_exec() || sched_core_enabled(rq)) && unlikely(prev != rq->donor);
+}
+
 /*
  * One user of this function is scx_bpf_sub_dispatch() which can be called
  * recursively as sub-sched dispatches nest. Always inline to reduce stack usage
@@ -33,8 +56,7 @@ scx_dispatch_sched(struct scx_sched *sch, struct rq *rq,
 	struct scx_dsp_ctx *dspc = &this_cpu_ptr(sch->pcpu)->dsp_ctx;
 	int nr_loops = SCX_DSP_MAX_LOOPS;
 	s32 cpu = cpu_of(rq);
-	bool prev_on_sch = (prev->sched_class == &ext_sched_class) &&
-		scx_task_on_sched(sch, prev);
+	bool prev_on_sch;
 
 	if (scx_consume_global_dsq(sch, rq))
 		return SCX_DSP_LOCAL;
@@ -82,7 +104,7 @@ scx_dispatch_sched(struct scx_sched *sch, struct rq *rq,
 	/*
 	 * scx_rq_online() can't be used. Its cpu_active() test goes false
 	 * before CPU hotplug waits for an RCU grace period, and
-	 * rq_offline_scx() moves this CPU's tasks to the local DSQ only after
+	 * handle_hotplug() moves this CPU's tasks to the local DSQ only after
 	 * the wait. The grace period can depend on those tasks running.
 	 */
 	if (unlikely(!SCX_HAS_OP(sch, dispatch)) || !(rq->scx.flags & SCX_RQ_ONLINE))
@@ -106,6 +128,11 @@ scx_dispatch_sched(struct scx_sched *sch, struct rq *rq,
 			rq->scx.sub_dispatch_prev = prev;
 #endif
 
+		/* the dispatch so far may have dropped the rq lock */
+		prev_on_sch = !scx_dsp_prev_stale(rq, prev) &&
+			      prev->sched_class == &ext_sched_class &&
+			      scx_task_on_sched(sch, prev);
+
 		SCX_CALL_OP(sch, dispatch, rq, scx_cpu_arg(cpu),
 			    prev_on_sch ? prev : NULL);
 
@@ -116,7 +143,9 @@ scx_dispatch_sched(struct scx_sched *sch, struct rq *rq,
 
 		scx_flush_dispatch_buf(sch, rq);
 
-		if ((prev->scx.flags & SCX_TASK_QUEUED) && prev->scx.slice)
+		/* ops.dispatch() and the flush may have dropped the rq lock */
+		if (!scx_dsp_prev_stale(rq, prev) &&
+		    (prev->scx.flags & SCX_TASK_QUEUED) && prev->scx.slice)
 			return SCX_DSP_PREV;
 		if (rq->scx.local_dsq.nr)
 			return SCX_DSP_LOCAL;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`cpu_active`
- 外部函数：`cpu_of`
- 外部函数：`rq_offline_scx`
- 外部函数：`scx_bpf_sub_dispatch`
- 外部函数：`scx_consume_global_dsq`
- 外部函数：`scx_cpu_arg`
- 外部函数：`scx_flush_dispatch_buf`
- 外部函数：`scx_rq_online`
- 外部函数：`scx_task_on_sched`
- 外部函数：`this_cpu_ptr`
- 外部函数：`unlikely`
- 大写宏：`CPU`
- 大写宏：`DSQ`
- 大写宏：`NULL`
- 大写宏：`RCU`
- 大写宏：`SCX_CALL_OP`
- 大写宏：`SCX_DSP_LOCAL`
- 大写宏：`SCX_DSP_MAX_LOOPS`
- 大写宏：`SCX_DSP_NONE`
- 大写宏：`SCX_DSP_PREV`
- 大写宏：`SCX_DSP_RETRY`
- 大写宏：`SCX_HAS_OP`
- 大写宏：`SCX_RQ_ONLINE`
- 大写宏：`SCX_TASK_QUEUED`
- 外部类型：`Always`
- 外部类型：`Its`
- 外部类型：`One`
- 外部类型：`The`
- 外部类型：`scx_dsp_ctx`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-691cbef772` → 本草稿移入 `cases/defect/auto-linux-691cbef772/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
