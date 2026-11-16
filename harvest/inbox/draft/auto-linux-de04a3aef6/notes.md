# auto-linux-de04a3aef6

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
| 外部依赖数（dep_count） | 39 |
| 编译错误数（gcc syntax-only） | 31（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #4b15991b3be43e9581aca489ec30a351310bb49d (https://github.com/torvalds/linux/commit/4b15991b3be43e9581aca489ec30a351310bb49d)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 268；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 4b15991b3be43e9581aca489ec30a351310bb49d Merge tag 'cgroup-for-7.4-rc2-fixes' of git://git.kernel.org/pub/scm/linux/kernel/git/tj/cgroup :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -265,7 +265,7 @@ static void update_if_frozen(struct cgroup_subsys_state *css)
 	rcu_read_unlock();
 
 	/* are all tasks frozen? */
-	css_task_iter_start(css, 0, &it);
+	css_task_iter_start(css, CSS_TASK_ITER_WITH_DEAD, &it);
 
 	while ((task = css_task_iter_next(&it))) {
 		if (freezing(task) && !frozen(task))
@@ -309,7 +309,7 @@ static void freeze_cgroup(struct freezer *freezer)
 	struct css_task_iter it;
 	struct task_struct *task;
 
-	css_task_iter_start(&freezer->css, 0, &it);
+	css_task_iter_start(&freezer->css, CSS_TASK_ITER_WITH_DEAD, &it);
 	while ((task = css_task_iter_next(&it)))
 		freeze_task(task);
 	css_task_iter_end(&it);
@@ -320,7 +320,7 @@ static void unfreeze_cgroup(struct freezer *freezer)
 	struct css_task_iter it;
 	struct task_struct *task;
 
-	css_task_iter_start(&freezer->css, 0, &it);
+	css_task_iter_start(&freezer->css, CSS_TASK_ITER_WITH_DEAD, &it);
 	while ((task = css_task_iter_next(&it)))
 		__thaw_task(task);
 	css_task_iter_end(&it);
@@ -382,6 +382,24 @@ static void freezer_change_state(struct freezer *freezer, bool freeze)
 	 * CGROUP_FREEZING_PARENT.
 	 */
 	mutex_lock(&freezer_mutex);
+
+	/*
+	 * A dead cgroup must not be frozen.
+	 *
+	 * rmdir thaws the cgroup from freezer_css_killed(), but freezer.state
+	 * stays writable until rmdir removes it a moment later. A FROZEN write
+	 * in that gap would freeze the cgroup again, and nothing could thaw it
+	 * after that.
+	 *
+	 * A freezer.state write doesn't take cgroup_mutex, but testing
+	 * cgroup_is_dead() without it is safe. rmdir marks the cgroup dead
+	 * before the thaw, so a write that takes freezer_mutex after the thaw
+	 * sees it dead. One that takes it before the thaw may still see the
+	 * cgroup alive, but the thaw then undoes its freeze.
+	 */
+	if (freeze && data_race(cgroup_is_dead(freezer->css.cgroup)))
+		goto out_unlock;
+
 	rcu_read_lock();
 	css_for_each_descendant_pre(pos, &freezer->css) {
 		struct freezer *pos_f = css_freezer(pos);
@@ -403,10 +421,25 @@ static void freezer_change_state(struct freezer *freezer, bool freeze)
 		css_put(pos);
 	}
 	rcu_read_unlock();
+out_unlock:
 	mutex_unlock(&freezer_mutex);
 	cpus_read_unlock();
 }
 
+/**
+ * freezer_css_killed - Thaw a freezer cgroup being removed
+ * @css: css being killed
+ *
+ * Exiting tasks hidden from cgroup.procs can still be in the cgroup when it is
+ * removed, and a frozen one has nothing left to thaw it once freezer.state is
+ * gone. Thaw the cgroup as a THAWED write to it would. A frozen ancestor keeps
+ * the tasks frozen through CGROUP_FREEZING_PARENT until the ancestor is thawed.
+ */
+static void freezer_css_killed(struct cgroup_subsys_state *css)
+{
+	freezer_change_state(css_freezer(css), false);
+}
+
 static ssize_t freezer_write(struct kernfs_open_file *of,
 			     char *buf, size_t nbytes, loff_t off)
 {
@@ -466,6 +499,7 @@ static struct cftype files[] = {
 struct cgroup_subsys freezer_cgrp_subsys = {
 	.css_alloc	= freezer_css_alloc,
 	.css_online	= freezer_css_online,
+	.css_killed	= freezer_css_killed,
 	.css_offline	= freezer_css_offline,
 	.css_free	= freezer_css_free,
 	.attach		= freezer_attach,
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__thaw_task`
- 外部函数：`cgroup_taskset_for_each`
- 外部函数：`container_of`
- 外部函数：`cpus_read_lock`
- 外部函数：`cpus_read_unlock`
- 外部函数：`css_put`
- 外部函数：`css_task_iter_end`
- 外部函数：`css_task_iter_next`
- 外部函数：`css_task_iter_start`
- 外部函数：`freeze_task`
- 外部函数：`freezing`
- 外部函数：`frozen`
- 外部函数：`kfree`
- 外部函数：`kzalloc_obj`
- 外部函数：`mutex_lock`
- 外部函数：`mutex_unlock`
- 外部函数：`rcu_read_lock`
- 外部函数：`rcu_read_unlock`
- 外部函数：`static_branch_dec_cpuslocked`
- 外部函数：`static_branch_inc_cpuslocked`
- 大写宏：`CGROUP_FREEZER_ONLINE`
- 大写宏：`CGROUP_FREEZING`
- 大写宏：`CGROUP_FREEZING_PARENT`
- 大写宏：`CGROUP_FROZEN`
- 大写宏：`ENOMEM`
- 大写宏：`ERR_PTR`
- 大写宏：`FREEZING`
- 大写宏：`FROZEN`
- 大写宏：`NULL`
- 大写宏：`THAWED`
- 外部类型：`For`
- 外部类型：`Make`
- 外部类型：`Tasks`
- 外部类型：`cgroup_subsys`
- 外部类型：`cgroup_subsys_state`
- 外部类型：`cgroup_taskset`
- 外部类型：`css_task_iter`
- 外部类型：`kernfs_open_file`
- 外部类型：`loff_t`
- 外部类型：`size_t`
- 外部类型：`ssize_t`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-de04a3aef6` → 本草稿移入 `cases/defect/auto-linux-de04a3aef6/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
