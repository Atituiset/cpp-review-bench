# auto-linux-b09094285d

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#e2c278bc37b52222e53d5f88971ffcfacd6c97be](https://github.com/torvalds/linux/commit/e2c278bc37b52222e53d5f88971ffcfacd6c97be) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-18 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 48 |
| 编译错误数（gcc syntax-only） | 16（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #e2c278bc37b52222e53d5f88971ffcfacd6c97be (https://github.com/torvalds/linux/commit/e2c278bc37b52222e53d5f88971ffcfacd6c97be)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 1321；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT e2c278bc37b52222e53d5f88971ffcfacd6c97be Merge tag 'trace-v7.3-rc7' of git://git.kernel.org/pub/scm/linux/kernel/git/trace/linux-trace :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -99,6 +99,12 @@ struct tracers {
 	struct tracer_flags	*flags;
 };
 
+/*
+ * Tracers that do not define their own flags use these empty flags,
+ * as tr->current_trace_flags must never be NULL.
+ */
+static struct tracer_flags no_tracer_flags;
+
 /*
  * To prevent the comm cache from being overwritten when no
  * tracing is active, only save the comm when a trace event
@@ -1318,7 +1324,12 @@ static int run_tracer_selftest(struct tracer *type)
 	tracing_reset_online_cpus(&tr->array_buffer);
 
 	tr->current_trace = type;
-	tr->current_trace_flags = type->flags ? : type->default_flags;
+	if (type->flags)
+		tr->current_trace_flags = type->flags;
+	else if (type->default_flags)
+		tr->current_trace_flags = type->default_flags;
+	else
+		tr->current_trace_flags = &no_tracer_flags;
 
 #ifdef CONFIG_TRACER_MAX_TRACE
 	if (tracer_uses_snapshot(type)) {
@@ -5089,7 +5100,12 @@ int tracing_set_tracer(struct trace_array *tr, const char *buf)
 			return ret;
 	}
 
-	tr->current_trace_flags = t->flags ? : t->tracer->flags;
+	if (t->flags)
+		tr->current_trace_flags = t->flags;
+	else if (t->tracer->flags)
+		tr->current_trace_flags = t->tracer->flags;
+	else
+		tr->current_trace_flags = &no_tracer_flags;
 
 	if (trace->init) {
 		ret = tracer_init(trace, tr);
@@ -5881,7 +5897,7 @@ static int tracing_buffer_meta_open(struct inode *inode, struct file *filp)
 
 	ret = ring_buffer_meta_seq_init(filp, tr->array_buffer.buffer, cpu);
 	if (ret < 0)
-		__trace_array_put(tr);
+		trace_array_put(tr);
 	return ret;
 }
 
@@ -9287,6 +9303,12 @@ static struct notifier_block trace_module_nb = {
 
 static __init void tracer_init_tracefs_work_func(struct work_struct *work)
 {
+	/*
+	 * Do not attempt to initialize tracefs if tracing was disabled or
+	 * failed to allocate
+	 */
+	if (tracing_disabled)
+		return;
 
 	event_trace_init();
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`event_trace_init`
- 外部函数：`flex_array_size`
- 外部函数：`guard`
- 外部函数：`init`
- 外部函数：`kfree_rcu`
- 外部函数：`memset`
- 外部函数：`module_for_each_mod`
- 外部函数：`queue_work`
- 外部函数：`ring_buffer_empty`
- 外部函数：`ring_buffer_meta_seq_init`
- 外部函数：`ring_buffer_normalize_time_stamp`
- 外部函数：`ring_buffer_record_disable`
- 外部函数：`ring_buffer_record_enable`
- 外部函数：`ring_buffer_reset`
- 外部函数：`ring_buffer_reset_online_cpus`
- 外部函数：`ring_buffer_time_stamp`
- 外部函数：`strscpy`
- 外部函数：`struct_size`
- 外部函数：`synchronize_rcu`
- 外部函数：`trace_clock_local`
- 外部函数：`tracer_uses_snapshot`
- 大写宏：`CONFIG_TRACER_MAX_TRACE`
- 大写宏：`CPU`
- 大写宏：`MODULE_NAME_LEN`
- 大写宏：`MOD_TEXT`
- 大写宏：`NULL`
- 大写宏：`READ_ONCE`
- 大写宏：`TRACE_ARRAY_FL_BOOT`
- 大写宏：`TRACE_ARRAY_FL_LAST_BOOT`
- 大写宏：`TRACE_ARRAY_FL_VMALLOC`
- 大写宏：`WARN_ON`
- 大写宏：`WRITE_ONCE`
- 外部类型：`Early`
- 外部类型：`If`
- 外部类型：`Make`
- 外部类型：`Need`
- 外部类型：`Only`
- 外部类型：`Reset`
- 外部类型：`Set`
- 外部类型：`To`
- 外部类型：`Using`
- 外部类型：`When`
- 外部类型：`array_buffer`
- 外部类型：`module`
- 外部类型：`trace_array`
- 外部类型：`trace_buffer`
- 外部类型：`trace_module_delta`
- 外部类型：`tracer`
- 外部类型：`tracer_flags`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-b09094285d` → 本草稿移入 `cases/defect/auto-linux-b09094285d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
