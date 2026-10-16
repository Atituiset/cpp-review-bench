# auto-linux-ad1cdad975

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#9aa622dbaab8addcbe6f65f17c7b4d613d270f05](https://github.com/torvalds/linux/commit/9aa622dbaab8addcbe6f65f17c7b4d613d270f05) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-16 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 36 |
| 编译错误数（gcc syntax-only） | 12（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #9aa622dbaab8addcbe6f65f17c7b4d613d270f05 (https://github.com/torvalds/linux/commit/9aa622dbaab8addcbe6f65f17c7b4d613d270f05)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 270；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 9aa622dbaab8addcbe6f65f17c7b4d613d270f05 Merge tag 'for-linus' of git://git.kernel.org/pub/scm/virt/kvm/kvm :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -265,11 +265,39 @@ noinstr u32 kvm_read_and_reset_apf_flags(void)
 }
 EXPORT_SYMBOL_FOR_KVM(kvm_read_and_reset_apf_flags);
 
+static __always_inline bool kvm_apf_reason_pending(void)
+{
+	return __this_cpu_read(async_pf_enabled) &&
+	       __this_cpu_read(apf_reason.flags);
+}
+
 noinstr bool __kvm_handle_async_pf(struct pt_regs *regs, u32 token)
 {
-	u32 flags = kvm_read_and_reset_apf_flags();
 	irqentry_state_t state;
+	u32 flags;
+
+	if (!kvm_apf_reason_pending())
+		return false;
+
+	/*
+	 * The guest does not enable KVM_ASYNC_PF_SEND_ALWAYS, so the host
+	 * delivers async #PF only to user mode (or as a #PF VM-exit, which KVM
+	 * reads itself). A #PF from kernel mode can still arrive between that
+	 * delivery and the read of the reason flags, inside an NMI that
+	 * interrupted it, for instance perf reading user memory for a call
+	 * chain. Such a fault is not async, so leave the async PF flags
+	 * untouched for the outer handler.  Outside NMI context, on the other
+	 * hand, async PF flags on a kernel-mode fault mean the host is broken.
+	 */
+	if (!user_mode(regs)) {
+		if (likely(in_nmi()))
+			return false;
+		state = irqentry_enter(regs);
+		instrumentation_begin();
+		panic("Host injected async #PF in kernel mode\n");
+	}
 
+	flags = kvm_read_and_reset_apf_flags();
 	if (!flags)
 		return false;
 
@@ -285,8 +313,6 @@ noinstr bool __kvm_handle_async_pf(struct pt_regs *regs, u32 token)
 		panic("Host injected async #PF in interrupt disabled region\n");
 
 	if (flags & KVM_PV_REASON_PAGE_NOT_PRESENT) {
-		if (unlikely(!(user_mode(regs))))
-			panic("Host injected async #PF in kernel mode\n");
 		/* Page is swapped out by the host. */
 		kvm_async_pf_task_wait_schedule(token);
 	} else {
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__this_cpu_read`
- 外部函数：`__this_cpu_write`
- 外部函数：`finish_swait`
- 外部函数：`hash_32`
- 外部函数：`hlist_add_head`
- 外部函数：`hlist_del`
- 外部函数：`hlist_entry`
- 外部函数：`hlist_unhashed`
- 外部函数：`init_swait_queue_head`
- 外部函数：`kfree`
- 外部函数：`local_irq_disable`
- 外部函数：`local_irq_enable`
- 外部函数：`lockdep_assert_irqs_disabled`
- 外部函数：`panic`
- 外部函数：`prepare_to_swait_exclusive`
- 外部函数：`raw_spin_lock`
- 外部函数：`raw_spin_unlock`
- 外部函数：`schedule`
- 外部函数：`smp_processor_id`
- 外部函数：`typeof`
- 外部函数：`unlikely`
- 外部函数：`user_mode`
- 大写宏：`APF`
- 大写宏：`DECLARE_SWAITQUEUE`
- 大写宏：`EXPORT_SYMBOL_FOR_KVM`
- 大写宏：`KVM_PV_REASON_PAGE_NOT_PRESENT`
- 大写宏：`NULL`
- 大写宏：`TASK_UNINTERRUPTIBLE`
- 外部类型：`Host`
- 外部类型：`Page`
- 外部类型：`The`
- 外部类型：`hlist_head`
- 外部类型：`hlist_node`
- 外部类型：`irqentry_state_t`
- 外部类型：`pt_regs`
- 外部类型：`raw_spinlock_t`
- 外部类型：`swait_queue_head`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-ad1cdad975` → 本草稿移入 `cases/defect/auto-linux-ad1cdad975/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
