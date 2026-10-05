# auto-linux-d2da5aa788

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#a27611f8994c9a4772156432badb533af33fd32d](https://github.com/torvalds/linux/commit/a27611f8994c9a4772156432badb533af33fd32d) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-05 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 137 |
| 编译错误数（gcc syntax-only） | 74（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #a27611f8994c9a4772156432badb533af33fd32d (https://github.com/torvalds/linux/commit/a27611f8994c9a4772156432badb533af33fd32d)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 9761；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT a27611f8994c9a4772156432badb533af33fd32d Merge tag 'perf-urgent-2026-10-04' of git://git.kernel.org/pub/scm/linux/kernel/git/tip/tip :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -7626,9 +7626,11 @@ static void perf_sigtrap(struct perf_event *event)
 {
 	/*
 	 * Both perf_pending_task() and perf_pending_irq() can race with the
-	 * task exiting.
+	 * task exiting or exec-ing. We can determine if such a race has
+	 * occurred by checking if perf_event_exit_task(), which will set
+	 * ctx->task to TASK_TOMBSTONE, has already been called.
 	 */
-	if (current->flags & PF_EXITING)
+	if (event->ctx->task == TASK_TOMBSTONE)
 		return;
 
 	/*
@@ -8129,10 +8131,15 @@ static void __perf_event_header__init_id(struct perf_sample_data *data,
 	}
 }
 
-void perf_event_header__init_id(struct perf_event_header *header,
-				struct perf_sample_data *data,
-				struct perf_event *event)
+void perf_event_header__init(struct perf_event_header *header,
+			     struct perf_sample_data *data,
+			     u32 type, u16 misc, u16 size,
+			     struct perf_event *event)
 {
+	header->type = type;
+	header->misc = misc;
+	header->size = size;
+
 	if (event->attr.sample_id_all) {
 		header->size += event->id_header_size;
 		__perf_event_header__init_id(data, event, event->attr.sample_type);
@@ -8969,17 +8976,16 @@ perf_event_read_event(struct perf_event *event,
 	struct perf_output_handle handle;
 	struct perf_sample_data sample;
 	struct perf_read_event read_event = {
-		.header = {
-			.type = PERF_RECORD_READ,
-			.misc = 0,
-			.size = sizeof(read_event) + event->read_size,
-		},
 		.pid = perf_event_pid(event, task),
 		.tid = perf_event_tid(event, task),
 	};
 	int ret;
 
-	perf_event_header__init_id(&read_event.header, &sample, event);
+	perf_event_header__init(&read_event.header, &sample,
+				PERF_RECORD_READ,
+				/* misc= */ 0,
+				sizeof(read_event) + event->read_size,
+				event);
 	ret = perf_output_begin(&handle, &sample, event, read_event.header.size);
 	if (ret)
 		return;
@@ -9220,6 +9226,7 @@ struct perf_task_event {
 		u32				ptid;
 		u64				time;
 	} event_id;
+	int new;
 };
 
 static int perf_event_task_match(struct perf_event *event)
@@ -9236,17 +9243,21 @@ static void perf_event_task_output(struct perf_event *event,
 	struct perf_output_handle handle;
 	struct perf_sample_data	sample;
 	struct task_struct *task = task_event->task;
-	int ret, size = task_event->event_id.header.size;
+	int ret;
 
 	if (!perf_event_task_match(event))
 		return;
 
-	perf_event_header__init_id(&task_event->event_id.header, &sample, event);
+	perf_event_header__init(&task_event->event_id.header, &sample,
+				task_event->new ? PERF_RECORD_FORK : PERF_RECORD_EXIT,
+				/* misc= */ 0,
+				sizeof(task_event->event_id),
+				event);
 
 	ret = perf_output_begin(&handle, &sample, event,
 				task_event->event_id.header.size);
 	if (ret)
-		goto out;
+		return;
 
 	task_event->event_id.pid = perf_event_pid(event, task);
 	task_event->event_id.tid = perf_event_tid(event, task);
@@ -9268,8 +9279,6 @@ static void perf_event_task_output(struct perf_event *event,
 	perf_event__output_id_sample(event, &handle, &sample);
 
 	perf_output_end(&handle);
-out:
-	task_event->event_id.header.size = size;
 }
 
 static void perf_event_task(struct task_struct *task,
@@ -9286,18 +9295,7 @@ static void perf_event_task(struct task_struct *task,
 	task_event = (struct perf_task_event){
 		.task	  = task,
 		.task_ctx = task_ctx,
-		.event_id    = {
-			.header = {
-				.type = new ? PERF_RECORD_FORK : PERF_RECORD_EXIT,
-				.misc = 0,
-				.size = sizeof(task_event.event_id),
-			},
-			/* .pid  */
-			/* .ppid */
-			/* .tid  */
-			/* .ptid */
-			/* .time */
-		},
+		.new	  = new,
 	};
 
 	perf_iterate_sb(perf_event_task_output,
@@ -9374,6 +9372,7 @@ struct perf_comm_event {
 		u32				pid;
 		u32				tid;
 	} event_id;
+	bool			exec;
 };
 
 static int perf_event_comm_match(struct perf_event *event)
@@ -9387,18 +9386,21 @@ static void perf_event_comm_output(struct perf_event *event,
 	struct perf_comm_event *comm_event = data;
 	struct perf_output_handle handle;
 	struct perf_sample_data sample;
-	int size = comm_event->event_id.header.size;
 	int ret
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__output_copy`
- 外部函数：`__task_pid_nr_ns`
- 外部函数：`arch_vma_name`
- 外部函数：`atomic_read`
- 外部函数：`build_id_parse_nofault`
- 外部函数：`cgroup_id`
- 外部函数：`cgroup_is_descendant`
- 外部函数：`clock`
- 外部函数：`d_path`
- 外部函数：`file_user_inode`
- 外部函数：`file_user_path`
- 外部函数：`is_vm_hugetlb_page`
- 外部函数：`kfree`
- 外部函数：`kmalloc`
- 外部函数：`memset`
- 外部函数：`name`
- 外部函数：`output`
- 外部函数：`perf_install_in_context`
- 外部函数：`perf_output_begin`
- 外部函数：`perf_output_end`
- 外部函数：`perf_output_put`
- 外部函数：`perf_pending_irq`
- 外部函数：`perf_pending_task`
- 外部函数：`perfmon_capable`
- 外部函数：`pid_alive`
- 外部函数：`preempt_disable`
- 外部函数：`preempt_enable`
- 外部函数：`raw_smp_processor_id`
- 外部函数：`rcu_dereference`
- 外部函数：`rcu_read_lock`
- 外部函数：`rcu_read_unlock`
- 外部函数：`s`
- 外部函数：`security_perf_event_open`
- 外部函数：`smp_load_acquire`
- 外部函数：`smp_processor_id`
- 外部函数：`strlen`
- 外部函数：`strscpy`
- 外部函数：`task_is_runnable`
- 外部函数：`this_cpu_ptr`
- 外部函数：`vma_is_initial_heap`
- 外部函数：`vma_is_initial_stack`
- 大写宏：`ALIGN`
- 大写宏：`BUILD_ID_SIZE_MAX`
- 大写宏：`CPU`
- 大写宏：`EACCES`
- 大写宏：`EXIT`
- 大写宏：`GFP_KERNEL`
- 大写宏：`IS_ALIGNED`
- 大写宏：`IS_ERR`
- 大写宏：`MAJOR`
- 大写宏：`MAP_HUGETLB`
- 大写宏：`MAP_LOCKED`
- 大写宏：`MAP_PRIVATE`
- 大写宏：`MAP_SHARED`
- 大写宏：`MINOR`
- 大写宏：`NR_NAMESPACES`
- 大写宏：`NULL`
- 大写宏：`PATH_MAX`
- 大写宏：`PERF_ATTACH_ITRACE`
- 大写宏：`PERF_EVENT_STATE_INACTIVE`
- 大写宏：`PERF_RECORD_AUX`
- 大写宏：`PERF_RECORD_AUX_OUTPUT_HW_ID`
- 大写宏：`PERF_RECORD_BPF_EVENT`
- 大写宏：`PERF_RECORD_CALLCHAIN_DEFERRED`
- 大写宏：`PERF_RECORD_CGROUP`
- 大写宏：`PERF_RECORD_COMM`
- 大写宏：`PERF_RECORD_EXIT`
- 大写宏：`PERF_RECORD_FORK`
- 大写宏：`PERF_RECORD_ITRACE_START`
- 大写宏：`PERF_RECORD_KSYMBOL`
- 大写宏：`PERF_RECORD_LOST_SAMPLES`
- 大写宏：`PERF_RECORD_MISC_COMM_EXEC`
- 大写宏：`PERF_RECORD_MISC_KERNEL`
- 大写宏：`PERF_RECORD_MISC_MMAP_BUILD_ID`
- 大写宏：`PERF_RECORD_MISC_MMAP_DATA`
- 大写宏：`PERF_RECORD_MISC_SWITCH_OUT`
- 大写宏：`PERF_RECORD_MISC_SWITCH_OUT_PREEMPT`
- 大写宏：`PERF_RECORD_MISC_USER`
- 大写宏：`PERF_RECORD_MMAP`
- 大写宏：`PERF_RECORD_MMAP2`
- 大写宏：`PERF_RECORD_NAMESPACES`
- 大写宏：`PERF_RECORD_READ`
- 大写宏：`PERF_RECORD_SWITCH`
- 大写宏：`PERF_RECORD_SWITCH_CPU_WIDE`
- 大写宏：`PERF_RECORD_TEXT_POKE`
- 大写宏：`PERF_RECORD_THROTTLE`
- 大写宏：`PERF_RECORD_UNTHROTTLE`
- 大写宏：`PERF_SAMPLE_CALLCHAIN`
- 大写宏：`PERF_SAMPLE_CPU`
- 大写宏：`PERF_SAMPLE_ID`
- 大写宏：`PERF_SAMPLE_IDENTIFIER`
- 大写宏：`PERF_SAMPLE_STREAM_ID`
- 大写宏：`PERF_SAMPLE_TID`
- 大写宏：`PERF_SAMPLE_TIME`
- 大写宏：`PERF_SECURITY_KERNEL`
- 大写宏：`PF_EXITING`
- 大写宏：`PIDTYPE_PID`
- 大写宏：`PIDTYPE_TGID`
- 大写宏：`PROT_EXEC`
- 大写宏：`PROT_READ`
- 大写宏：`PROT_WRITE`
- 大写宏：`TASK_COMM_LEN`
- 大写宏：`VM_EXEC`
- 大写宏：`VM_LOCKED`
- 大写宏：`VM_MAYSHARE`
- 大写宏：`VM_READ`
- 大写宏：`VM_WRITE`
- 大写宏：`XXX`
- 外部类型：`An`
- 外部类型：`Both`
- 外部类型：`Cgroup`
- 外部类型：`However`
- 外部类型：`If`
- 外部类型：`Only`
- 外部类型：`See`
- 外部类型：`Since`
- 外部类型：`Skip`
- 外部类型：`The`
- 外部类型：`cgroup`
- 外部类型：`dev_t`
- 外部类型：`file`
- 外部类型：`inode`
- 外部类型：`perf_bpf_event`
- 外部类型：`perf_cgroup_event`
- 外部类型：`perf_cpu_context`
- 外部类型：`perf_event`
- 外部类型：`perf_event_context`
- 外部类型：`perf_event_header`
- 外部类型：`perf_namespaces_event`
- 外部类型：`perf_ns_link_info`
- 外部类型：`perf_output_handle`
- 外部类型：`perf_sample_data`
- 外部类型：`perf_switch_event`
- 外部类型：`perf_task_event`
- 外部类型：`pid_type`
- 外部类型：`pmu_event_list`
- 外部类型：`size_t`
- 外部类型：`task_struct`
- 外部类型：`unwind_stacktrace`
- 外部类型：`unwind_work`
- 外部类型：`vm_area_struct`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-d2da5aa788` → 本草稿移入 `cases/defect/auto-linux-d2da5aa788/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
