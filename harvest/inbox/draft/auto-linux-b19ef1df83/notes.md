# auto-linux-b19ef1df83

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#bdeb84ed24e543930487f03f5243bbf83aebaf31](https://github.com/torvalds/linux/commit/bdeb84ed24e543930487f03f5243bbf83aebaf31) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-07 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 123 |
| 编译错误数（gcc syntax-only） | 125（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #bdeb84ed24e543930487f03f5243bbf83aebaf31 (https://github.com/torvalds/linux/commit/bdeb84ed24e543930487f03f5243bbf83aebaf31)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 4326；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT bdeb84ed24e543930487f03f5243bbf83aebaf31 Merge tag 'printk-for-7.4-fixups' of git://git.kernel.org/pub/scm/linux/kernel/git/printk/linux :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -3792,6 +3792,13 @@ static struct syscore printk_syscore = {
  * If appropriate, start nbcon kthreads and set @printk_kthreads_running.
  * If any kthreads fail to start, those consoles are unregistered.
  *
+ * Called after consoles are registered or unregistered.
+ *
+ * This MUST NOT be called within an hlist_for_each_entry_safe(console_list)
+ * loop because it can unregister multiple consoles if any kthreads fail to
+ * start. That would lead to use-after-free scenarios within the
+ * hlist_for_each_entry_safe(console_list) loop of the caller.
+ *
  * Must be called under console_list_lock().
  */
 static void printk_kthreads_check_locked(void)
@@ -4230,7 +4237,20 @@ void register_console(struct console *newcon)
 }
 EXPORT_SYMBOL(register_console);
 
-/* Must be called under console_list_lock(). */
+/*
+ * Unregister the specified console.
+ *
+ * This can be called safely from within an
+ * hlist_for_each_entry_safe(console_list) loop because it unregisters
+ * _only_ the specified console.
+ *
+ * Afterwards the caller should call printk_kthreads_check_locked() since
+ * unregistering a console may require printer threads to start/stop. Note
+ * that printk_kthreads_check_locked() cannot be called safely from within
+ * an hlist_for_each_entry_safe(console_list) loop.
+ *
+ * Must be called under console_list_lock().
+ */
 static int unregister_console_locked(struct console *console)
 {
 	bool use_device_lock = (console->flags & CON_NBCON) && console->write_atomic;
@@ -4322,9 +4342,6 @@ static int unregister_console_locked(struct console *console)
 	if (console->exit)
 		res = console->exit(console);
 
-	/* Changed console list, may require printer threads to start/stop. */
-	printk_kthreads_check_locked();
-
 	return res;
 }
 
@@ -4333,7 +4350,12 @@ int unregister_console(struct console *console)
 	int res;
 
 	console_list_lock();
+
 	res = unregister_console_locked(console);
+
+	/* Changed console list, may require printer threads to start/stop. */
+	printk_kthreads_check_locked();
+
 	console_list_unlock();
 	return res;
 }
@@ -4472,6 +4494,10 @@ static int __init printk_late_init(void)
 
 		unregister_console_locked(con);
 	}
+
+	/* Changed console list, may require printer threads to start/stop. */
+	printk_kthreads_check_locked();
+
 	console_list_unlock();
 
 	ret = cpuhp_setup_state_nocalls(CPUHP_PRINTK_DEAD, "printk:dead", NULL,
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__up_console_sem`
- 外部函数：`_braille_register_console`
- 外部函数：`bootconsole`
- 外部函数：`con_printk`
- 外部函数：`cond_resched`
- 外部函数：`console_call_setup`
- 外部函数：`console_is_usable`
- 外部函数：`console_list_unlock`
- 外部函数：`console_lock`
- 外部函数：`console_lock_spinning_disable_and_check`
- 外部函数：`console_lock_spinning_enable`
- 外部函数：`console_prepend_dropped`
- 外部函数：`console_srcu_read_flags`
- 外部函数：`console_srcu_read_lock`
- 外部函数：`console_srcu_read_unlock`
- 外部函数：`console_srcu_write_flags`
- 外部函数：`console_sysfs_notify`
- 外部函数：`cpuhp_setup_state_nocalls`
- 外部函数：`default`
- 外部函数：`device_lock`
- 外部函数：`device_unlock`
- 外部函数：`down`
- 外部函数：`down_console_sem`
- 外部函数：`down_trylock_console_sem`
- 外部函数：`exit`
- 外部函数：`flush`
- 外部函数：`force_legacy_kthread`
- 外部函数：`hlist_add_behind_rcu`
- 外部函数：`hlist_add_head_rcu`
- 外部函数：`hlist_empty`
- 外部函数：`hlist_entry`
- 外部函数：`match`
- 外部函数：`might_sleep`
- 外部函数：`msleep`
- 外部函数：`mutex_acquire`
- 外部函数：`mutex_lock`
- 外部函数：`mutex_unlock`
- 外部函数：`nbcon_alloc`
- 外部函数：`nbcon_free`
- 外部函数：`nbcon_legacy_emit_next_record`
- 外部函数：`nbcon_seq_force`
- 外部函数：`nbcon_seq_read`
- 外部函数：`panic_in_progress`
- 外部函数：`panic_on_other_cpu`
- 外部函数：`pr_info`
- 外部函数：`prb_next_seq`
- 外部函数：`prb_read_valid`
- 外部函数：`printk_get_console_flush_type`
- 外部函数：`printk_get_next_message`
- 外部函数：`printk_legacy_allow_spinlock_enter`
- 外部函数：`printk_legacy_allow_spinlock_exit`
- 外部函数：`printk_safe_enter_irqsave`
- 外部函数：`printk_safe_exit_irqrestore`
- 外部函数：`register_console`
- 外部函数：`setup`
- 外部函数：`start_critical_timings`
- 外部函数：`stop_critical_timings`
- 外部函数：`strcmp`
- 外部函数：`unregister_console_locked`
- 外部函数：`up_console_sem`
- 外部函数：`write`
- 大写宏：`BUILD_BUG_ON`
- 大写宏：`CON_BOOT`
- 大写宏：`CON_BRL`
- 大写宏：`CON_CONSDEV`
- 大写宏：`CON_ENABLED`
- 大写宏：`CON_EXTENDED`
- 大写宏：`CON_NBCON`
- 大写宏：`CON_PRINTBUFFER`
- 大写宏：`CPU`
- 大写宏：`CPUHP_PRINTK_DEAD`
- 大写宏：`ENOENT`
- 大写宏：`EXPORT_SYMBOL`
- 大写宏：`KERN_INFO`
- 大写宏：`NULL`
- 大写宏：`RCU`
- 大写宏：`SPCR`
- 大写宏：`SRCU`
- 大写宏：`WARN`
- 外部类型：`Abort`
- 外部类型：`Accept`
- 外部类型：`Allow`
- 外部类型：`Begin`
- 外部类型：`Braille`
- 外部类型：`By`
- 外部类型：`Changed`
- 外部类型：`Console`
- 外部类型：`Do`
- 外部类型：`Either`
- 外部类型：`Ensure`
- 外部类型：`Flush`
- 外部类型：`Flushing`
- 外部类型：`Get`
- 外部类型：`Hold`
- 外部类型：`If`
- 外部类型：`Interrupts`
- 外部类型：`Just`
- 外部类型：`More`
- 外部类型：`Must`
- 外部类型：`No`
- 外部类型：`Nope`
- 外部类型：`Not`
- 外部类型：`Note`
- 外部类型：`On`
- 外部类型：`Only`
- 外部类型：`Put`
- 外部类型：`Re`
- 外部类型：`See`
- 外部类型：`Since`
- 外部类型：`Skip`
- 外部类型：`Some`
- 外部类型：`Synchronize`
- 外部类型：`The`
- 外部类型：`There`
- 外部类型：`Therefore`
- 外部类型：`This`
- 外部类型：`Too`
- 外部类型：`Track`
- 外部类型：`Use`
- 外部类型：`While`
- 外部类型：`With`
- 外部类型：`Write`
- 外部类型：`console`
- 外部类型：`console_cmdline`
- 外部类型：`console_flush_type`
- 外部类型：`hlist_node`
- 外部类型：`printk_message`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-b19ef1df83` → 本草稿移入 `cases/defect/auto-linux-b19ef1df83/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
