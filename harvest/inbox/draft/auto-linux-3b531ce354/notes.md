# auto-linux-3b531ce354

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#e767a4ea70a3992c37ed604157d32f0dfbf9b1e3](https://github.com/torvalds/linux/commit/e767a4ea70a3992c37ed604157d32f0dfbf9b1e3) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-03 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 40 |
| 编译错误数（gcc syntax-only） | 4（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #e767a4ea70a3992c37ed604157d32f0dfbf9b1e3 (https://github.com/torvalds/linux/commit/e767a4ea70a3992c37ed604157d32f0dfbf9b1e3)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 341；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT e767a4ea70a3992c37ed604157d32f0dfbf9b1e3 Merge tag 'probes-fixes-v7.3-rc5' of git://git.kernel.org/pub/scm/linux/kernel/git/trace/linux-trace :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -47,6 +47,10 @@ static struct rhltable fprobe_ip_table;
 static DEFINE_MUTEX(fprobe_mutex);
 static struct fgraph_ops fprobe_graph_ops;
 
+DEFINE_LOCK_GUARD_0(rcu_sched_notrace,
+		    rcu_read_lock_sched_notrace(),
+		    rcu_read_unlock_sched_notrace())
+
 static u32 fprobe_node_hashfn(const void *data, u32 len, u32 seed)
 {
 	return hash_ptr(*(unsigned long **)data, 32);
@@ -329,16 +333,14 @@ static void fprobe_ftrace_entry(unsigned long ip, unsigned long parent_ip,
 	struct fprobe *fp;
 	int bit;
 
+	if (!rcu_is_watching())
+		return;
+
 	bit = ftrace_test_recursion_trylock(ip, parent_ip);
 	if (bit < 0)
 		return;
 
-	/*
-	 * ftrace_test_recursion_trylock() disables preemption, but
-	 * rhltable_lookup() checks whether rcu_read_lcok is held.
-	 * So we take rcu_read_lock() here.
-	 */
-	rcu_read_lock();
+	guard(rcu_sched_notrace)();
 	head = rhltable_lookup(&fprobe_ip_table, &ip, fprobe_rht_params);
 
 	rhl_for_each_entry_rcu(node, pos, head, hlist) {
@@ -353,7 +355,6 @@ static void fprobe_ftrace_entry(unsigned long ip, unsigned long parent_ip,
 		else
 			__fprobe_handler(ip, parent_ip, fp, fregs, NULL);
 	}
-	rcu_read_unlock();
 	ftrace_test_recursion_unlock(bit);
 }
 NOKPROBE_SYMBOL(fprobe_ftrace_entry);
@@ -567,10 +568,13 @@ static int fprobe_fgraph_entry(struct ftrace_graph_ent *trace, struct fgraph_ops
 	struct fprobe *fp;
 	int used, ret;
 
+	if (!rcu_is_watching())
+		return 0;
+
 	if (WARN_ON_ONCE(!fregs))
 		return 0;
 
-	guard(rcu)();
+	guard(rcu_sched_notrace)();
 	head = rhltable_lookup(&fprobe_ip_table, &func, fprobe_rht_params);
 	reserved_words = 0;
 	rhl_for_each_entry_rcu(node, pos, head, hlist) {
@@ -665,13 +669,16 @@ static void fprobe_return(struct ftrace_graph_ret *trace,
 	int size, curr;
 	int size_words;
 
+	if (!rcu_is_watching())
+		return;
+
 	fgraph_data = (unsigned long *)fgraph_retrieve_data(gops->idx, &size);
 	if (WARN_ON_ONCE(!fgraph_data))
 		return;
 	size_words = SIZE_IN_LONG(size);
 	ret_ip = ftrace_regs_get_instruction_pointer(fregs);
 
-	preempt_disable_notrace();
+	guard(rcu_sched_notrace)();
 
 	curr = 0;
 	while (size_words > curr) {
@@ -687,7 +694,6 @@ static void fprobe_return(struct ftrace_graph_ret *trace,
 		}
 		curr += size;
 	}
-	preempt_enable_notrace();
 }
 NOKPROBE_SYMBOL(fprobe_return);
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`arch_decode_fprobe_header_fp`
- 外部函数：`arch_decode_fprobe_header_size`
- 外部函数：`entry_handler`
- 外部函数：`exit_handler`
- 外部函数：`fgraph_retrieve_data`
- 外部函数：`fprobe_disabled`
- 外部函数：`fprobe_shared_with_kprobes`
- 外部函数：`ftrace_regs_get_instruction_pointer`
- 外部函数：`ftrace_test_recursion_trylock`
- 外部函数：`ftrace_test_recursion_unlock`
- 外部函数：`guard`
- 外部函数：`hash_ptr`
- 外部函数：`kprobe_busy_begin`
- 外部函数：`kprobe_busy_end`
- 外部函数：`kprobe_running`
- 外部函数：`lockdep_is_held`
- 外部函数：`preempt_disable_notrace`
- 外部函数：`preempt_enable_notrace`
- 外部函数：`rcu_read_lock`
- 外部函数：`rcu_read_unlock`
- 外部函数：`rhltable_lookup`
- 外部函数：`unlikely`
- 大写宏：`DEFINE_MUTEX`
- 大写宏：`NOKPROBE_SYMBOL`
- 大写宏：`NULL`
- 大写宏：`READ_ONCE`
- 大写宏：`WARN_ON_ONCE`
- 外部类型：`Documentation`
- 外部类型：`See`
- 外部类型：`Share`
- 外部类型：`So`
- 外部类型：`This`
- 外部类型：`fgraph_ops`
- 外部类型：`fprobe`
- 外部类型：`fprobe_hlist`
- 外部类型：`fprobe_hlist_node`
- 外部类型：`ftrace_graph_ret`
- 外部类型：`ftrace_ops`
- 外部类型：`ftrace_regs`
- 外部类型：`hlist_head`
- 外部类型：`rhlist_head`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-3b531ce354` → 本草稿移入 `cases/defect/auto-linux-3b531ce354/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
