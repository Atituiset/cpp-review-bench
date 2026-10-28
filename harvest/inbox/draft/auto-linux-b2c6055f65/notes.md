# auto-linux-b2c6055f65

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
| 外部依赖数（dep_count） | 62 |
| 编译错误数（gcc syntax-only） | 24（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #59a02493f3b68fd0ad68c661393f680bcca5a85b (https://github.com/torvalds/linux/commit/59a02493f3b68fd0ad68c661393f680bcca5a85b)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 194；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 59a02493f3b68fd0ad68c661393f680bcca5a85b Merge tag 'trace-v7.4' of git://git.kernel.org/pub/scm/linux/kernel/git/trace/linux-trace :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -191,7 +191,7 @@ struct trace_array;
 struct trace_array_cpu {
 	local_t			disabled;
 
-	unsigned long		entries;
+	unsigned long		capacity;
 	unsigned long		saved_latency;
 	unsigned long		critical_start;
 	unsigned long		critical_end;
@@ -731,7 +731,7 @@ ssize_t tracing_nsecs_read(unsigned long *ptr, char __user *ubuf,
 ssize_t tracing_nsecs_write(unsigned long *ptr, const char __user *ubuf,
 			    size_t cnt, loff_t *ppos);
 
-void trace_set_buffer_entries(struct array_buffer *buf, unsigned long val);
+void trace_set_buffer_capacity(struct array_buffer *buf, unsigned long val);
 
 /*
  * Should be used after trace_array_get(), trace_types_lock
@@ -1823,7 +1823,8 @@ static inline bool is_string_field(struct ftrace_event_field *field)
 	       field->filter_type == FILTER_RDYN_STRING ||
 	       field->filter_type == FILTER_STATIC_STRING ||
 	       field->filter_type == FILTER_PTR_STRING ||
-	       field->filter_type == FILTER_COMM;
+	       field->filter_type == FILTER_COMM ||
+	       field->filter_type == FILTER_WITHIN;
 }
 
 static inline bool is_function_field(struct ftrace_event_field *field)
@@ -2286,7 +2287,6 @@ static inline const char *get_syscall_name(int syscall)
 void trace_event_init(void);
 void trace_event_update_all(struct trace_eval_map **map, int len, struct module *mod);
 /* Used from boot time tracer */
-extern int ftrace_set_clr_event(struct trace_array *tr, char *buf, int set);
 extern int trigger_process_regex(struct trace_event_file *file, char *buff);
 #else
 static inline void __init trace_event_init(void) { }
@@ -2506,3 +2506,8 @@ static inline int rv_init_interface(void)
 	})
 
 #endif /* _LINUX_KERNEL_TRACE_H */
+
+#if IS_ENABLED(CONFIG_KUNIT)
+void ftrace_parse_event_string(char *buf, char **match, char **sub, char **event, char **mod,
+			       bool handle_filter, char **filter);
+#endif
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`int`
- 外部函数：`ssize_t`
- 外部函数：`void`
- 大写宏：`CONFIG_DYNAMIC_FTRACE`
- 大写宏：`CONFIG_FSNOTIFY`
- 大写宏：`CONFIG_FTRACE_SYSCALLS`
- 大写宏：`CONFIG_FUNCTION_GRAPH_TRACER`
- 大写宏：`CONFIG_FUNCTION_TRACER`
- 大写宏：`CONFIG_MODULES`
- 大写宏：`CONFIG_TRACER_MAX_TRACE`
- 大写宏：`CONFIG_TRACER_SNAPSHOT`
- 外部类型：`All`
- 外部类型：`But`
- 外部类型：`CPUs`
- 外部类型：`If`
- 外部类型：`It`
- 外部类型：`Mask`
- 外部类型：`NR_syscalls`
- 外部类型：`On`
- 外部类型：`Some`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`When`
- 外部类型：`Will`
- 外部类型：`arch_spinlock_t`
- 外部类型：`cond_snapshot`
- 外部类型：`cpumask_var_t`
- 外部类型：`dentry`
- 外部类型：`eventfs_inode`
- 外部类型：`fgraph_ops`
- 外部类型：`ftrace_ops`
- 外部类型：`irq_work`
- 外部类型：`list_head`
- 外部类型：`local_t`
- 外部类型：`raw_spinlock_t`
- 外部类型：`rcu_head`
- 外部类型：`spinlock_t`
- 外部类型：`trac`
- 外部类型：`trace_buffer`
- 外部类型：`trace_event_file`
- 外部类型：`trace_iterator`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-b2c6055f65` → 本草稿移入 `cases/defect/auto-linux-b2c6055f65/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
