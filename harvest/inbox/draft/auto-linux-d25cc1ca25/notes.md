# auto-linux-d25cc1ca25

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#e4fb7e5e8e7e9edc167c3057802e440b77dcb856](https://github.com/torvalds/linux/commit/e4fb7e5e8e7e9edc167c3057802e440b77dcb856) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-10 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 92 |
| 编译错误数（gcc syntax-only） | 171（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #e4fb7e5e8e7e9edc167c3057802e440b77dcb856 (https://github.com/torvalds/linux/commit/e4fb7e5e8e7e9edc167c3057802e440b77dcb856)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 1814；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT e4fb7e5e8e7e9edc167c3057802e440b77dcb856 Merge tag 'probes-fixes-v7.4-rc2' of git://git.kernel.org/pub/scm/linux/kernel/git/trace/linux-trace :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -633,8 +633,12 @@ static void do_unoptimize_kprobes(struct list_head *unopt_list,
 	list_for_each_entry_safe(op, tmp, free_list, list) {
 		/* Switching from detour code to origin */
 		op->kp.flags &= ~KPROBE_FLAG_OPTIMIZED;
-		/* Disarm probes if marked disabled and not gone */
-		if (kprobe_disabled(&op->kp) && !kprobe_gone(&op->kp))
+		/*
+		 * Disarm probes if marked disabled and not gone. Unhashed probes
+		 * were disarmed before they were unhashed.
+		 */
+		if (kprobe_disabled(&op->kp) && !kprobe_gone(&op->kp) &&
+		    !hlist_unhashed(&op->kp.hlist))
 			arch_disarm_kprobe(&op->kp);
 		if (kprobe_unused(&op->kp)) {
 			/*
@@ -1002,9 +1006,10 @@ static void unoptimize_kprobe(struct kprobe *p, bool force)
 				 * for cooling down (or freeing if unused).
 				 */
 				force_unoptimize_kprobe(op);
+				/* Disarm probes if marked disabled and not gone */
+				if (kprobe_disabled(&op->kp) && !kprobe_gone(&op->kp))
+					arch_disarm_kprobe(&op->kp);
 				if (kprobe_unused(&op->kp)) {
-					if (!kprobe_gone(&op->kp))
-						arch_disarm_kprobe(&op->kp);
 					hlist_del_init_rcu(&op->kp.hlist);
 					list_move(&op->list, &cur->freeing_list);
 				} else {
@@ -1056,6 +1061,10 @@ static int reuse_unused_kprobe(struct kprobe *ap)
 	 */
 	op = container_of(ap, struct optimized_kprobe, kp);
 	WARN_ON_ONCE(list_empty(&op->list));
+	/* Already disarmed, the caller arms it only for an enabled probe */
+	if (optprobe_queued_cooling(op))
+		return 0;
+
 	/* Enable the probe again */
 	ap->flags &= ~KPROBE_FLAG_DISABLED;
 	/* Optimize it again. (remove from 'op->list') */
@@ -1568,7 +1577,8 @@ void kprobe_busy_end(void)
 /* Add the new probe to 'ap->list'. */
 static int add_new_kprobe(struct kprobe *ap, struct kprobe *p)
 {
-	if (p->post_handler)
+	/* A reused cooling probe is already unoptimized */
+	if (p->post_handler && !kprobe_unused(ap))
 		unoptimize_kprobe(ap, true);	/* Fall back to normal kprobe */
 
 	list_add_rcu(&p->list, &ap->list);
@@ -1793,52 +1803,61 @@ _kprobe_addr(kprobe_opcode_t *addr, const char *symbol_name,
 	return addr;
 }
 
-static kprobe_opcode_t *kprobe_addr(struct kprobe *p)
-{
-	bool on_func_entry;
-
-	return _kprobe_addr(p->addr, p->symbol_name, p->offset, &on_func_entry);
-}
-
 /*
  * Check the 'p' is valid and return the aggregator kprobe
  * at the same address.
  */
-static struct kprobe *__get_valid_kprobe(struct kprobe *p)
+static struct kprobe *__get_valid_kprobe_at(struct kprobe *p, kprobe_opcode_t *addr)
 {
+	struct hlist_head *head;
 	struct kprobe *ap, *list_p;
 
 	lockdep_assert_held(&kprobe_mutex);
 
-	ap = get_kprobe(p->addr);
-	if (unlikely(!ap))
+	if (unlikely(!addr))
 		return NULL;
 
-	if (p == ap)
-		return ap;
-
-	list_for_each_entry(list_p, &ap->list, list)
-		if (list_p == p)
-		/* kprobe p is a valid probe */
+	head = &kprobe_table[hash_ptr(addr, KPROBE_HASH_BITS)];
+	hlist_for_each_entry_rcu(ap, head, hlist,
+				 lockdep_is_held(&kprobe_mutex)) {
+		if (ap == p) {
+			p->addr = addr;
 			return ap;
+		}
+		if (ap->addr == addr && kprobe_aggrprobe(ap)) {
+			list_for_each_entry(list_p, &ap->list, list) {
+				if (list_p == p) {
+					p->addr = addr;
+					return ap;
+				}
+			}
+		}
+	}
 
 	return NULL;
 }
 
-/*
- * Warn and return error if the kprobe is being re-registered since
- * there must be a software bug.
- */
-static inline int warn_kprobe_rereg(struct kprobe *p)
+static inline struct kprobe *__get_valid_kprobe(struct kprobe *p)
+{
+	return __get_valid_kprobe_at(p, p->addr);
+}
+
+/* Return error if the kprobe is being re-registered */
+static inline int check_kprobe_rereg_at(struct kprobe *p, kprobe_opcode_t *addr)
 {
 	guard(mutex)(&kprobe_mutex);
 
-	if (WARN_ON_ONCE(__get_valid_kprobe(p)))
+	if (__get_valid_kprobe_at(p, addr))
 		return -EINVAL;
 
 	return 0;
 }
 
+static inline int check_kprobe_rereg(struct kprobe *p)
+{
+	return check_kprobe_rereg_at(p, p->addr);
+}
+
 static int check_ftrace_location(struct kprobe *p)
 {
 	unsigned long addr = (unsigned long)p->ad
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__disarm_kprobe`
- 外部函数：`__disarm_kprobe_ftrace`
- 外部函数：`__this_cpu_write`
- 外部函数：`arch_disarm_kprobe`
- 外部函数：`arch_remove_kprobe`
- 外部函数：`arch_remove_optimized_kprobe`
- 外部函数：`arch_unoptimize_kprobe`
- 外部函数：`container_of`
- 外部函数：`down`
- 外部函数：`ftrace`
- 外部函数：`ftrace_set_filter_ip`
- 外部函数：`guard`
- 外部函数：`hash_ptr`
- 外部函数：`hlist_del_init_rcu`
- 外部函数：`hlist_del_rcu`
- 外部函数：`kallsyms_lookup_name`
- 外部函数：`kallsyms_lookup_size_offset`
- 外部函数：`kfree`
- 外部函数：`kprobe_disabled`
- 外部函数：`kprobe_disarmed`
- 外部函数：`kprobe_ftrace`
- 外部函数：`kprobe_gone`
- 外部函数：`likely`
- 外部函数：`list_add_rcu`
- 外部函数：`list_del`
- 外部函数：`list_del_rcu`
- 外部函数：`list_empty`
- 外部函数：`list_entry`
- 外部函数：`list_entry_is_head`
- 外部函数：`list_is_singular`
- 外部函数：`list_move`
- 外部函数：`lockdep_assert_cpus_held`
- 外部函数：`lockdep_assert_held`
- 外部函数：`lockdep_is_held`
- 外部函数：`objpool_fini`
- 外部函数：`optimize_kprobe`
- 外部函数：`pS`
- 外部函数：`pre_handler`
- 外部函数：`rethook_free`
- 外部函数：`synchronize_rcu`
- 外部函数：`unlikely`
- 外部函数：`unoptimize_kprobe`
- 外部函数：`unregister_ftrace_function`
- 大写宏：`CONFIG_KRETPROBE_ON_RETHOOK`
- 大写宏：`EINVAL`
- 大写宏：`ENOENT`
- 大写宏：`ERR_PTR`
- 大写宏：`IS_ERR`
- 大写宏：`KPROBE_FLAG_DISABLED`
- 大写宏：`KPROBE_FLAG_OPTIMIZED`
- 大写宏：`NOT`
- 大写宏：`NOTE`
- 大写宏：`NULL`
- 大写宏：`PTR_ERR`
- 大写宏：`WARN`
- 大写宏：`WARN_ON`
- 大写宏：`WARN_ONCE`
- 大写宏：`WARN_ON_ONCE`
- 外部类型：`Add`
- 外部类型：`Check`
- 外部类型：`Disable`
- 外部类型：`Disarm`
- 外部类型：`Don`
- 外部类型：`Enable`
- 外部类型：`Even`
- 外部类型：`Failed`
- 外部类型：`Fall`
- 外部类型：`For`
- 外部类型：`Get`
- 外部类型：`If`
- 外部类型：`Input`
- 外部类型：`No`
- 外部类型：`Note`
- 外部类型：`Now`
- 外部类型：`Optimize`
- 外部类型：`Otherwise`
- 外部类型：`Output`
- 外部类型：`Remove`
- 外部类型：`Since`
- 外部类型：`So`
- 外部类型：`Switching`
- 外部类型：`Then`
- 外部类型：`This`
- 外部类型：`Try`
- 外部类型：`Warn`
- 外部类型：`ftrace_ops`
- 外部类型：`hlist_head`
- 外部类型：`kprobe`
- 外部类型：`kprobe_opcode_t`
- 外部类型：`kretprobe`
- 外部类型：`kretprobe_holder`
- 外部类型：`optimized_kprobe`
- 外部类型：`pt_regs`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-d25cc1ca25` → 本草稿移入 `cases/defect/auto-linux-d25cc1ca25/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
