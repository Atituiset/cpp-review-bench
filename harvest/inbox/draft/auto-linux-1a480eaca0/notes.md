# auto-linux-1a480eaca0

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#59a02493f3b68fd0ad68c661393f680bcca5a85b](https://github.com/torvalds/linux/commit/59a02493f3b68fd0ad68c661393f680bcca5a85b) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-28 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 127 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #59a02493f3b68fd0ad68c661393f680bcca5a85b (https://github.com/torvalds/linux/commit/59a02493f3b68fd0ad68c661393f680bcca5a85b)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 1448；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 59a02493f3b68fd0ad68c661393f680bcca5a85b Merge tag 'trace-v7.4' of git://git.kernel.org/pub/scm/linux/kernel/git/trace/linux-trace :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -25,6 +25,8 @@
 #include <linux/delay.h>
 #include <linux/btf.h>
 
+#include <kunit/visibility.h>
+
 #include <trace/events/sched.h>
 #include <trace/syscall.h>
 
@@ -201,6 +203,9 @@ static int trace_define_generic_fields(void)
 	__generic_field(char *, comm, FILTER_COMM);
 	__generic_field(char *, stacktrace, FILTER_STACKTRACE);
 	__generic_field(char *, STACKTRACE, FILTER_STACKTRACE);
+#ifdef CONFIG_STACKTRACE
+	__generic_field(char *, within, FILTER_WITHIN);
+#endif
 
 	return ret;
 }
@@ -241,6 +246,10 @@ int trace_event_get_offsets(struct trace_event_call *call)
 	struct list_head *head;
 
 	head = trace_get_fields(call);
+
+	if (list_empty(head))
+		return sizeof(struct trace_entry);
+
 	/*
 	 * head->next points to the last field with the largest offset,
 	 * since it was added last by trace_define_field()
@@ -941,6 +950,7 @@ struct event_mod_load {
 	char			*match;
 	char			*system;
 	char			*event;
+	char			*filter;
 };
 
 static void free_event_mod(struct event_mod_load *event_mod)
@@ -950,6 +960,7 @@ static void free_event_mod(struct event_mod_load *event_mod)
 	kfree(event_mod->match);
 	kfree(event_mod->system);
 	kfree(event_mod->event);
+	kfree(event_mod->filter);
 	kfree(event_mod);
 }
 
@@ -991,7 +1002,7 @@ static int remove_cache_mod(struct trace_array *tr, const char *mod,
 }
 
 static int cache_mod(struct trace_array *tr, const char *mod, int set,
-		     const char *match, const char *system, const char *event)
+		     const char *match, const char *system, const char *event, const char *filter)
 {
 	struct event_mod_load *event_mod;
 
@@ -1030,6 +1041,12 @@ static int cache_mod(struct trace_array *tr, const char *mod, int set,
 			goto out_free;
 	}
 
+	if (filter) {
+		event_mod->filter = kstrdup(filter, GFP_KERNEL);
+		if (!event_mod->filter)
+			goto out_free;
+	}
+
 	list_add(&event_mod->list, &tr->mod_events);
 
 	return 0;
@@ -1042,7 +1059,7 @@ static int cache_mod(struct trace_array *tr, const char *mod, int set,
 #else /* CONFIG_MODULES */
 static inline void clear_mod_events(struct trace_array *tr) { }
 static int cache_mod(struct trace_array *tr, const char *mod, int set,
-		     const char *match, const char *system, const char *event)
+		     const char *match, const char *system, const char *event, const char *filter)
 {
 	return -EINVAL;
 }
@@ -1351,7 +1368,7 @@ static void remove_event_file_dir(struct trace_event_file *file)
 static int
 __ftrace_set_clr_event_nolock(struct trace_array *tr, const char *match,
 			      const char *sub, const char *event, int set,
-			      const char *mod)
+			      const char *mod, char *filter)
 {
 	struct trace_event_file *file;
 	struct trace_event_call *call;
@@ -1401,7 +1418,11 @@ __ftrace_set_clr_event_nolock(struct trace_array *tr, const char *match,
 		if (event && strcmp(event, name) != 0)
 			continue;
 
-		ret = ftrace_event_enable_disable(file, set);
+		if (filter)
+			ret = apply_event_filter(file, filter);
+
+		if (!filter || !ret)
+			ret = ftrace_event_enable_disable(file, set);
 
 		/*
 		 * Save the first error and return that. Some events
@@ -1419,41 +1440,50 @@ __ftrace_set_clr_event_nolock(struct trace_array *tr, const char *match,
 	 * check if the module was loaded. If it wasn't cache it.
 	 */
 	if (module && ret == -EINVAL && !eret)
-		ret = cache_mod(tr, module, set, match, sub, event);
+		ret = cache_mod(tr, module, set, match, sub, event, filter);
 
 	return ret;
 }
 
 static int __ftrace_set_clr_event(struct trace_array *tr, const char *match,
 				  const char *sub, const char *event, int set,
-				  const char *mod)
+				  const char *mod, char *filter)
 {
 	int ret;
 
 	if (trace_array_is_readonly(tr))
 		return -EACCES;
 
 	mutex_lock(&event_mutex);
-	ret = __ftrace_set_clr_event_nolock(tr, match, sub, event, set, mod);
+	ret = __ftrace_set_clr_event_nolock(tr, match, sub, event, set, mod, filter);
 	mutex_unlock(&event_mutex);
 
 	return ret;
 }
 
-int ftrace_set_clr_event(struct trace_array *tr, char *buf, int set)
+VIS
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__ftrace_event_enable_disable`
- 外部函数：`__generic_field`
- 外部函数：`__set_current_state`
- 外部函数：`atomic_dec_return`
- 外部函数：`atomic_inc_return`
- 外部函数：`atomic_read`
- 外部函数：`clear_bit`
- 外部函数：`filter_assign_type`
- 外部函数：`for_each_possible_cpu`
- 外部函数：`is_signed_type`
- 外部函数：`kfree`
- 外部函数：`kmalloc`
- 外部函数：`kmem_cache_alloc`
- 外部函数：`kthread_run`
- 外部函数：`kthread_should_stop`
- 外部函数：`kthread_stop`
- 外部函数：`list_add`
- 外部函数：`list_del`
- 外部函数：`lockdep_is_held`
- 外部函数：`msleep`
- 外部函数：`mutex_lock`
- 外部函数：`mutex_unlock`
- 外部函数：`per_cpu_ptr`
- 外部函数：`pid_type_enabled`
- 外部函数：`pr_info`
- 外部函数：`pr_warn`
- 外部函数：`rcu_assign_pointer`
- 外部函数：`rcu_dereference_protected`
- 外部函数：`rcu_dereference_sched`
- 外部函数：`reg`
- 外部函数：`schedule`
- 外部函数：`schedule_on_each_cpu`
- 外部函数：`set_bit`
- 外部函数：`set_current_state`
- 外部函数：`spin_lock`
- 外部函数：`spin_lock_irq`
- 外部函数：`spin_unlock`
- 外部函数：`spin_unlock_irq`
- 外部函数：`still_need_pid_events`
- 外部函数：`strcmp`
- 外部函数：`strlen`
- 外部函数：`strsep`
- 外部函数：`strstr`
- 外部函数：`this_cpu_read`
- 外部函数：`this_cpu_write`
- 外部函数：`top_trace_array`
- 外部函数：`trace_array_is_readonly`
- 外部函数：`trace_buffered_event_disable`
- 外部函数：`trace_buffered_event_enable`
- 外部函数：`trace_event_name`
- 外部函数：`trace_get_fields`
- 外部函数：`trace_ignore_this_task`
- 外部函数：`trace_pid_list_free`
- 外部函数：`tracepoint_synchronize_unregister`
- 外部函数：`tracing_start_cmdline_record`
- 外部函数：`tracing_start_tgid_record`
- 外部函数：`tracing_stop_cmdline_record`
- 外部函数：`tracing_stop_tgid_record`
- 外部函数：`udelay`
- 外部函数：`unregister_trace_sched_switch`
- 外部函数：`unregister_trace_sched_wakeup`
- 外部函数：`unregister_trace_sched_wakeup_new`
- 外部函数：`unregister_trace_sched_waking`
- 大写宏：`CONFIG_MODULES`
- 大写宏：`EACCES`
- 大写宏：`EINVAL`
- 大写宏：`ENODEV`
- 大写宏：`ENOENT`
- 大写宏：`ENOMEM`
- 大写宏：`EVENT_FILE_FL_ENABLED`
- 大写宏：`EVENT_FILE_FL_ENABLED_BIT`
- 大写宏：`EVENT_FILE_FL_PID_FILTER_BIT`
- 大写宏：`EVENT_FILE_FL_RECORDED_CMD`
- 大写宏：`EVENT_FILE_FL_RECORDED_CMD_BIT`
- 大写宏：`EVENT_FILE_FL_RECORDED_TGID`
- 大写宏：`EVENT_FILE_FL_RECORDED_TGID_BIT`
- 大写宏：`EVENT_FILE_FL_SOFT_DISABLED`
- 大写宏：`EVENT_FILE_FL_SOFT_DISABLED_BIT`
- 大写宏：`EVENT_FILE_FL_WAS_ENABLED_BIT`
- 大写宏：`EXPORT_SYMBOL_GPL`
- 大写宏：`FILTER_COMM`
- 大写宏：`FILTER_OTHER`
- 大写宏：`FILTER_STACKTRACE`
- 大写宏：`GFP_KERNEL`
- 大写宏：`IS_ERR`
- 大写宏：`NULL`
- 大写宏：`RECORD_CMD`
- 大写宏：`RECORD_TGID`
- 大写宏：`SOFT_DISABLED`
- 大写宏：`SOFT_DISABLE_BIT`
- 大写宏：`STACKTRACE`
- 大写宏：`TASK_INTERRUPTIBLE`
- 大写宏：`TASK_RUNNING`
- 大写宏：`TRACE_ITER`
- 大写宏：`TRACE_NO_PIDS`
- 大写宏：`TRACE_PIDS`
- 大写宏：`TRACE_REG_REGISTER`
- 大写宏：`TRACE_REG_UNREGISTER`
- 大写宏：`WARN_ON`
- 大写宏：`WARN_ON_ONCE`
- 大写宏：`WAS_ENABLED`
- 外部类型：`Allow`
- 外部类型：`But`
- 外部类型：`Could`
- 外部类型：`Disable`
- 外部类型：`Do`
- 外部类型：`Enable`
- 外部类型：`Failed`
- 外部类型：`If`
- 外部类型：`Keep`
- 外部类型：`Make`
- 外部类型：`Modules`
- 外部类型：`Nothing`
- 外部类型：`Put`
- 外部类型：`Running`
- 外部类型：`Save`
- 外部类型：`Sched`
- 外部类型：`Set`
- 外部类型：`Some`
- 外部类型：`Testing`
- 外部类型：`That`
- 外部类型：`Wait`
- 外部类型：`When`
- 外部类型：`ftrace_event_field`
- 外部类型：`list_head`
- 外部类型：`task_struct`
- 外部类型：`trace_array`
- 外部类型：`trace_event_call`
- 外部类型：`trace_event_file`
- 外部类型：`trace_pid_list`
- 外部类型：`work_struct`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-1a480eaca0` → 本草稿移入 `cases/defect/auto-linux-1a480eaca0/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
