# auto-linux-e7d89b3393

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#22430ae5d90ab288b0ee2ad99ae941f4a666b694](https://github.com/torvalds/linux/commit/22430ae5d90ab288b0ee2ad99ae941f4a666b694) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-07 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 112 |
| 编译错误数（gcc syntax-only） | 149（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #22430ae5d90ab288b0ee2ad99ae941f4a666b694 (https://github.com/torvalds/linux/commit/22430ae5d90ab288b0ee2ad99ae941f4a666b694)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 22430ae5d90ab288b0ee2ad99ae941f4a666b694 Merge tag 'printk-for-7.3-rc7' of git://git.kernel.org/pub/scm/linux/kernel/git/printk/linux :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1887,6 +1887,7 @@ bool nbcon_device_try_acquire(struct console *con)
 
 	memset(ctxt, 0, sizeof(*ctxt));
 	ctxt->console	= con;
+	/* Keep in sync with nbcon_braille_try_acquire(). */
 	ctxt->prio	= NBCON_PRIO_NORMAL;
 
 	if (!nbcon_context_try_acquire(ctxt, false))
@@ -2002,3 +2003,81 @@ void nbcon_kdb_release(struct nbcon_write_context *wctxt)
 	 */
 	__nbcon_atomic_flush_pending_con(ctxt->console, prb_next_reserve_seq(prb));
 }
+
+/**
+ * nbcon_is_braille - Checks whether the nbcon write context is using Braille console
+ *
+ * @wctxt:	checked nbcon write context
+ *
+ * Return: True when the write context is associated with a Braille console.
+ *	   Otherwise, return false.
+ *
+ * Context: Can be called in any context but only when Braille console is
+ *	registered and the struct console could not disappear.
+ */
+bool nbcon_write_context_is_braille(struct nbcon_write_context *wctxt)
+{
+	struct nbcon_context *ctxt = &ACCESS_PRIVATE(wctxt, ctxt);
+	struct console *con = ctxt->console;
+
+	return con && con->flags & CON_BRL;
+}
+EXPORT_SYMBOL_GPL(nbcon_write_context_is_braille);
+
+/**
+ * nbcon_braille_try_acquire - Try to acquire nbcon console for braille_write()
+ *
+ * @con:	The nbcon console to acquire
+ * @wctxt:	The nbcon write context to be used on success
+ *
+ * Context:	braille_write() for emitting a single buffer on Braille console.
+ *
+ * Return:	True if the console was acquired. False otherwise.
+ *
+ * Braille console is not registered as a proper printk consoles. Instead,
+ * it is integrated with the graphical virtual terminal.
+ *
+ * This function is going to synchronize the Braille write against other
+ * operations on the used serial port. The port can be used also for a user
+ * input but printk() won't emit the messages there directly. It means
+ * the other operations will get synchronized using nbcon_device_try_acquire().
+ */
+bool nbcon_braille_try_acquire(struct console *con,
+			   struct nbcon_write_context *wctxt)
+{
+	struct nbcon_context *ctxt = &ACCESS_PRIVATE(wctxt, ctxt);
+	bool success;
+
+	memset(ctxt, 0, sizeof(*ctxt));
+	ctxt->console = con;
+	/* Keep in sync with nbcon_device_try_acquire(). */
+	if (panic_on_this_cpu())
+		ctxt->prio = NBCON_PRIO_PANIC;
+	else
+		ctxt->prio = NBCON_PRIO_NORMAL;
+
+	success = nbcon_context_try_acquire(ctxt, false);
+	/*
+	 * Try hard in panic, otherwise the Braille console would
+	 * miss a text on the related VT.
+	 */
+	if (panic_on_this_cpu() && !success) {
+		ctxt->allow_unsafe_takeover = true;
+		success = nbcon_context_try_acquire(ctxt, false);
+	}
+
+	return success;
+}
+
+/**
+ * nbcon_braille_release - Release the nbcon console
+ *
+ * @wctxt:	The nbcon write context initialized by a successful
+ *		nbcon_braille_try_acquire()
+ */
+void nbcon_braille_release(struct nbcon_write_context *wctxt)
+{
+	struct nbcon_context *ctxt = &ACCESS_PRIVATE(wctxt, ctxt);
+
+	nbcon_context_release(ctxt);
+}
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__u64seq_to_ulseq`
- 外部函数：`__ulseq_to_u64seq`
- 外部函数：`atomic_long_read`
- 外部函数：`atomic_long_try_cmpxchg`
- 外部函数：`atomic_read`
- 外部函数：`atomic_try_cmpxchg`
- 外部函数：`kdb_printf_on_this_cpu`
- 外部函数：`panic_on_other_cpu`
- 外部函数：`reacquire`
- 外部函数：`smp_processor_id`
- 大写宏：`ACCESS_PRIVATE`
- 大写宏：`CPU`
- 大写宏：`EBUSY`
- 大写宏：`EMERGENCY`
- 大写宏：`EPERM`
- 大写宏：`NBCON_PRIO_NONE`
- 大写宏：`PANIC`
- 大写宏：`WARN_ON_ONCE`
- 外部类型：`Another`
- 外部类型：`As`
- 外部类型：`CPUs`
- 外部类型：`Check`
- 外部类型：`Console`
- 外部类型：`Event`
- 外部类型：`Handover`
- 外部类型：`However`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Lower`
- 外部类型：`Note`
- 外部类型：`One`
- 外部类型：`Only`
- 外部类型：`Panic`
- 外部类型：`Second`
- 外部类型：`The`
- 外部类型：`There`
- 外部类型：`This`
- 外部类型：`console`
- 外部类型：`nbcon_context`
- 外部类型：`nbcon_state`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-e7d89b3393` → 本草稿移入 `cases/defect/auto-linux-e7d89b3393/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
