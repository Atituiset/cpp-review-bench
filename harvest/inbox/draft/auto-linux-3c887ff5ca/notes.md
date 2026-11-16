# auto-linux-3c887ff5ca

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#4b15991b3be43e9581aca489ec30a351310bb49d](https://github.com/torvalds/linux/commit/4b15991b3be43e9581aca489ec30a351310bb49d) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-16 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 131 |
| 编译错误数（gcc syntax-only） | 280（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #4b15991b3be43e9581aca489ec30a351310bb49d (https://github.com/torvalds/linux/commit/4b15991b3be43e9581aca489ec30a351310bb49d)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 3047；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 4b15991b3be43e9581aca489ec30a351310bb49d Merge tag 'cgroup-for-7.4-rc2-fixes' of git://git.kernel.org/pub/scm/linux/kernel/git/tj/cgroup :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1244,17 +1244,17 @@ static bool isolated_cpu_update(int new_prs, int cpu)
 }
 
 /*
- * isolated_cpus_update - Update the isolated_cpus mask
- * @old_prs: old partition_root_state
- * @new_prs: new partition_root_state
- * @xcpus: exclusive CPUs with state change
+ * isolated_cpus_update - Set the isolation state of @xcpus
+ * @new_prs: partition root state that @xcpus will belong to
+ * @xcpus: exclusive CPUs whose isolation state may change
+ *
+ * CPUs isolated at boot time are kept isolated.
  */
-static void isolated_cpus_update(int old_prs, int new_prs, struct cpumask *xcpus)
+static void isolated_cpus_update(int new_prs, struct cpumask *xcpus)
 {
 	bool updated = false;
 	int cpu;
 
-	WARN_ON_ONCE(old_prs == new_prs);
 	lockdep_assert_held(&callback_lock);
 	lockdep_assert_held(&cpuset_mutex);
 	for_each_cpu(cpu, xcpus)
@@ -1263,45 +1263,6 @@ static void isolated_cpus_update(int old_prs, int new_prs, struct cpumask *xcpus
 		update_housekeeping = true;
 }
 
-/*
- * isolated_cpus_release - Remove the relevant CPUs in @xcpus from isolated_cpus
- * @cs: the current cpuset
- * @xcpus: the set of effective xcpus to be considered for removal
- *
- * This function is called when an isolated partition is invalidated or when
- * some of its effective xcpus are to be removed from isolated_cpus due to
- * changes in parent cpuset. The given @xcpus must be modifiable as the function
- * will update its value to determine the right set of CPUs to be removed.
- */
-static void isolated_cpus_release(struct cpuset *cs, struct cpumask *xcpus)
-{
-	struct cpuset *ancestor = parent_cs(cs);
-	struct cpuset *iancestor = NULL;
-
-	/*
-	 * Search the furthest isolated partition root ancestor available
-	 * until top_cpuset or a remote partition root is hit.
-	 */
-	for (; ancestor; ancestor = parent_cs(ancestor)) {
-		if (ancestor->partition_root_state == PRS_ISOLATED)
-			iancestor = ancestor;
-		if (is_remote_partition(ancestor))
-			break;
-	}
-
-	/*
-	 * If a furthest isolated partition acnestor is found, use it to
-	 * determine which of the CPUs in @xcpus should be removed from
-	 * isolated_cpus. Otherwise, all of them will be removed.
-	 */
-	if (iancestor && !cpumask_andnot(xcpus, xcpus, iancestor->effective_xcpus))
-		return; /* No CPU should be removed from isolated_cpus */
-
-	spin_lock_irq(&callback_lock);
-	isolated_cpus_update(PRS_ISOLATED, PRS_ROOT, xcpus);
-	spin_unlock_irq(&callback_lock);
-}
-
 /*
  * partition_xcpus_add - Add new exclusive CPUs to partition
  * @new_prs: new partition_root_state
@@ -1315,16 +1276,17 @@ static void partition_xcpus_add(int new_prs, struct cpuset *parent,
 {
 	WARN_ON_ONCE(new_prs < 0);
 	lockdep_assert_held(&callback_lock);
+
+	if (cpumask_empty(xcpus))
+		return;
+
 	if (!parent)
 		parent = &top_cpuset;
 
-
 	if (parent == &top_cpuset)
 		cpumask_or(subpartitions_cpus, subpartitions_cpus, xcpus);
 
-	if (new_prs != parent->partition_root_state)
-		isolated_cpus_update(parent->partition_root_state, new_prs,
-				     xcpus);
+	isolated_cpus_update(new_prs, xcpus);
 
 	cpumask_andnot(parent->effective_cpus, parent->effective_cpus, xcpus);
 }
@@ -1342,15 +1304,18 @@ static void partition_xcpus_del(int old_prs, struct cpuset *parent,
 {
 	WARN_ON_ONCE(old_prs < 0);
 	lockdep_assert_held(&callback_lock);
+
+	if (cpumask_empty(xcpus))
+		return;
+
 	if (!parent)
 		parent = &top_cpuset;
 
 	if (parent == &top_cpuset)
 		cpumask_andnot(subpartitions_cpus, subpartitions_cpus, xcpus);
 
-	if (old_prs != parent->partition_root_state)
-		isolated_cpus_update(old_prs, parent->partition_root_state,
-				     xcpus);
+	/* CPUs given back, including those of descendants, take parent's state */
+	isolated_cpus_update(parent->partition_root_state, xcpus);
 
 	cpumask_or(parent->effective_cpus, parent->effective_cpus, xcpus);
 	cpumask_and(parent->effective_cpus, parent->effective_cpus, cpu_active_mask);
@@ -1449,6 +1414,48 @@ static bool prstate_housekeeping_conflict(int new_prs, int parent_prs,
 	retur
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`alloc_cpumask_var`
- 外部函数：`assign_bit`
- 外部函数：`capable`
- 外部函数：`cgroup_file_notify`
- 外部函数：`cgroup_has_tasks`
- 外部函数：`cgroup_subsys_on_dfl`
- 外部函数：`cpumask_and`
- 外部函数：`cpumask_andnot`
- 外部函数：`cpumask_clear`
- 外部函数：`cpumask_copy`
- 外部函数：`cpumask_empty`
- 外部函数：`cpumask_equal`
- 外部函数：`cpumask_first_and_and`
- 外部函数：`cpumask_intersects`
- 外部函数：`cpumask_or`
- 外部函数：`cpumask_subset`
- 外部函数：`cpumask_weight_andnot`
- 外部函数：`cpuset_handle_hotplug`
- 外部函数：`cpuset_hotplug_update_tasks`
- 外部函数：`cpuset_is_populated`
- 外部函数：`cs_is_populated`
- 外部函数：`css_put`
- 外部函数：`css_rightmost_descendant`
- 外部函数：`css_task_iter_end`
- 外部函数：`css_task_iter_next`
- 外部函数：`css_task_iter_start`
- 外部函数：`css_tryget_online`
- 外部函数：`effective_xcpus`
- 外部函数：`for_each_cpu`
- 外部函数：`free_cpumask_var`
- 外部函数：`housekeeping_cpumask`
- 外部函数：`housekeeping_enabled`
- 外部函数：`is_sched_load_balance`
- 外部函数：`lockdep_assert_held`
- 外部函数：`partition`
- 外部函数：`rcu_read_lock`
- 外部函数：`rcu_read_unlock`
- 外部函数：`return_isolated_cpus`
- 外部函数：`set_cpus_allowed_ptr`
- 外部函数：`spin_lock_irq`
- 外部函数：`spin_unlock_irq`
- 外部函数：`swap`
- 外部函数：`task_cpu_possible_mask`
- 外部函数：`update_cpumask`
- 外部函数：`validate_partition`
- 大写宏：`CAP_SYS_ADMIN`
- 大写宏：`CONFIG_CPUSETS_V1`
- 大写宏：`CPU`
- 大写宏：`CS_SCHED_LOAD_BALANCE`
- 大写宏：`GFP_KERNEL`
- 大写宏：`HK_TYPE_DOMAIN`
- 大写宏：`HK_TYPE_DOMAIN_BOOT`
- 大写宏：`HK_TYPE_KERNEL_NOISE`
- 大写宏：`IS_ENABLED`
- 大写宏：`NULL`
- 大写宏：`PERR_ACCESS`
- 大写宏：`PERR_CPUSEMPTY`
- 大写宏：`PERR_HKEEPING`
- 大写宏：`PERR_INVCPUS`
- 大写宏：`PERR_INVPARENT`
- 大写宏：`PERR_ISOPARENT`
- 大写宏：`PERR_NOCPUS`
- 大写宏：`PERR_NONE`
- 大写宏：`PERR_NOTEXCL`
- 大写宏：`PERR_NOTPART`
- 大写宏：`PF_KTHREAD`
- 大写宏：`PF_NO_SETAFFINITY`
- 大写宏：`PRS`
- 大写宏：`RCU`
- 大写宏：`READ_ONCE`
- 大写宏：`WARN_ON`
- 大写宏：`WARN_ON_ONCE`
- 大写宏：`WRITE_ONCE`
- 外部类型：`Add`
- 外部类型：`Adding`
- 外部类型：`Additions`
- 外部类型：`At`
- 外部类型：`CPUs`
- 外部类型：`Call`
- 外部类型：`Capture`
- 外部类型：`Change`
- 外部类型：`Check`
- 外部类型：`Compute`
- 外部类型：`Convert`
- 外部类型：`Deleting`
- 外部类型：`Empty`
- 外部类型：`Enabling`
- 外部类型：`Except`
- 外部类型：`Exit`
- 外部类型：`For`
- 外部类型：`Force`
- 外部类型：`Handle`
- 外部类型：`If`
- 外部类型：`Invalidate`
- 外部类型：`It`
- 外部类型：`Make`
- 外部类型：`May`
- 外部类型：`Need`
- 外部类型：`Newly`
- 外部类型：`No`
- 外部类型：`Now`
- 外部类型：`On`
- 外部类型：`Otherwise`
- 外部类型：`Partition`
- 外部类型：`Propagate`
- 外部类型：`Ready`
- 外部类型：`Rebuild`
- 外部类型：`Recompute`
- 外部类型：`Remote`
- 外部类型：`Remove`
- 外部类型：`Reset`
- 外部类型：`Return`
- 外部类型：`Search`
- 外部类型：`Skip`
- 外部类型：`So`
- 外部类型：`Switching`
- 外部类型：`The`
- 外部类型：`There`
- 外部类型：`This`
- 外部类型：`Update`
- 外部类型：`Validate`
- 外部类型：`Warn`
- 外部类型：`We`
- 外部类型：`When`
- 外部类型：`With`
- 外部类型：`cgroup_subsys_state`
- 外部类型：`cpumask`
- 外部类型：`cpumask_var_t`
- 外部类型：`cpuset`
- 外部类型：`css_task_iter`
- 外部类型：`prs_errcode`
- 外部类型：`task_struct`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-3c887ff5ca` → 本草稿移入 `cases/defect/auto-linux-3c887ff5ca/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
