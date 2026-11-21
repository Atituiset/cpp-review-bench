# auto-linux-f751a2f065

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#7d51f37ed48e11a0a073ec428abe9d61c4449072](https://github.com/torvalds/linux/commit/7d51f37ed48e11a0a073ec428abe9d61c4449072) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-21 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 76 |
| 编译错误数（gcc syntax-only） | 131（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #7d51f37ed48e11a0a073ec428abe9d61c4449072 (https://github.com/torvalds/linux/commit/7d51f37ed48e11a0a073ec428abe9d61c4449072)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 209；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 7d51f37ed48e11a0a073ec428abe9d61c4449072 Merge tag 'watchdog-for-v7.4-rc4' of git://git.kernel.org/pub/scm/linux/kernel/git/groeck/linux-staging :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -55,6 +55,7 @@
 static dev_t watchdog_devt;
 /* Reference to watchdog device behind /dev/watchdog */
 static struct watchdog_core_data *old_wd_data;
+static DEFINE_MUTEX(old_wd_data_lock);
 
 static struct kthread_worker *watchdog_kworker;
 
@@ -80,6 +81,9 @@ static inline bool watchdog_need_worker(struct watchdog_device *wdd)
 	unsigned int hm = wdd->max_hw_heartbeat_ms;
 	unsigned int t = wdd->timeout * 1000;
 
+	if (test_bit(_WDOG_SUSPENDED, &wdd->wd_data->status))
+		return false;
+
 	/*
 	 * A worker to generate heartbeat requests is needed if all of the
 	 * following conditions are true.
@@ -153,9 +157,10 @@ static int __watchdog_ping(struct watchdog_device *wdd)
 	now = ktime_get();
 
 	if (ktime_after(earliest_keepalive, now)) {
-		hrtimer_start(&wd_data->timer,
-			      ktime_sub(earliest_keepalive, now),
-			      HRTIMER_MODE_REL_HARD);
+		if (!test_bit(_WDOG_SUSPENDED, &wd_data->status))
+			hrtimer_start(&wd_data->timer,
+				      ktime_sub(earliest_keepalive, now),
+				      HRTIMER_MODE_REL_HARD);
 		return 0;
 	}
 
@@ -206,7 +211,7 @@ static bool watchdog_worker_should_ping(struct watchdog_core_data *wd_data)
 {
 	struct watchdog_device *wdd = wd_data->wdd;
 
-	if (!wdd)
+	if (!wdd || test_bit(_WDOG_SUSPENDED, &wd_data->status))
 		return false;
 
 	if (watchdog_active(wdd))
@@ -259,10 +264,14 @@ static int watchdog_start(struct watchdog_device *wdd)
 
 	started_at = ktime_get();
 	if (watchdog_hw_running(wdd) && wdd->ops->ping) {
+		set_bit(WDOG_ACTIVE, &wdd->status);
+		wd_data->last_keepalive = started_at;
 		err = __watchdog_ping(wdd);
 		if (err == 0) {
-			set_bit(WDOG_ACTIVE, &wdd->status);
 			watchdog_hrtimer_pretimeout_start(wdd);
+		} else {
+			clear_bit(WDOG_ACTIVE, &wdd->status);
+			watchdog_update_worker(wdd);
 		}
 	} else {
 		err = wdd->ops->start(wdd);
@@ -307,6 +316,7 @@ static int watchdog_stop(struct watchdog_device *wdd)
 		clear_bit(WDOG_HW_RUNNING, &wdd->status);
 		err = wdd->ops->stop(wdd);
 		if (err < 0) {
+			set_bit(WDOG_HW_RUNNING, &wdd->status);
 			pr_err("watchdog%d: Failed to stop watchdog: %pe\n",
 			       wdd->id, ERR_PTR(err));
 		}
@@ -869,17 +879,32 @@ static int watchdog_open(struct inode *inode, struct file *file)
 	int err;
 
 	/* Get the corresponding watchdog device */
-	if (imajor(inode) == MISC_MAJOR)
+	if (imajor(inode) == MISC_MAJOR) {
+		mutex_lock(&old_wd_data_lock);
 		wd_data = old_wd_data;
-	else
+		if (!wd_data) {
+			mutex_unlock(&old_wd_data_lock);
+			return -ENODEV;
+		}
+		mutex_lock(&wd_data->lock);
+		mutex_unlock(&old_wd_data_lock);
+	} else {
 		wd_data = container_of(inode->i_cdev, struct watchdog_core_data,
 				       cdev);
+		mutex_lock(&wd_data->lock);
+	}
 
 	/* the watchdog is single open! */
-	if (test_and_set_bit(_WDOG_DEV_OPEN, &wd_data->status))
+	if (test_and_set_bit(_WDOG_DEV_OPEN, &wd_data->status)) {
+		mutex_unlock(&wd_data->lock);
 		return -EBUSY;
+	}
 
 	wdd = wd_data->wdd;
+	if (!wdd) {
+		err = -ENODEV;
+		goto out_clear;
+	}
 
 	/*
 	 * If the /dev/watchdog device is open, we don't want the module
@@ -909,13 +934,17 @@ static int watchdog_open(struct inode *inode, struct file *file)
 	 */
 	wd_data->open_deadline = KTIME_MAX;
 
+	mutex_unlock(&wd_data->lock);
+
 	/* dev/watchdog is a virtual (and thus non-seekable) filesystem */
 	return stream_open(inode, file);
 
 out_mod:
-	module_put(wd_data->wdd->ops->owner);
+	if (!hw_running)
+		module_put(wd_data->wdd->ops->owner);
 out_clear:
 	clear_bit(_WDOG_DEV_OPEN, &wd_data->status);
+	mutex_unlock(&wd_data->lock);
 	return err;
 }
 
@@ -1022,6 +1051,7 @@ static const struct class watchdog_class = {
 static int watchdog_cdev_register(struct watchdog_device *wdd)
 {
 	struct watchdog_core_data *wd_data;
+	bool hw_running;
 	int err;
 
 	wd_data = kzalloc_obj(struct watchdog_core_data);
@@ -1033,6 +1063,7 @@ static int watchdog_cdev_register(struct watchdog_device *wdd)
 	wdd->wd_data = wd_data;
 
 	if (IS_ERR_OR_NULL(watchdog_kworker)) {
+		wdd->wd_data = NULL;
 		kfre
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`clear_bit`
- 外部函数：`container_of`
- 外部函数：`hrtimer_cancel`
- 外部函数：`hrtimer_start`
- 外部函数：`imajor`
- 外部函数：`ktime_add`
- 外部函数：`ktime_after`
- 外部函数：`ktime_before`
- 外部函数：`ktime_get`
- 外部函数：`ktime_set`
- 外部函数：`ktime_sub`
- 外部函数：`kzalloc_obj`
- 外部函数：`min_not_zero`
- 外部函数：`module_put`
- 外部函数：`ms_to_ktime`
- 外部函数：`ping`
- 外部函数：`pr_err`
- 外部函数：`pr_info`
- 外部函数：`set_bit`
- 外部函数：`start`
- 外部函数：`stop`
- 外部函数：`stream_open`
- 外部函数：`test_and_set_bit`
- 外部函数：`test_bit`
- 外部函数：`trace_watchdog_ping`
- 外部函数：`trace_watchdog_start`
- 外部函数：`trace_watchdog_stop`
- 外部函数：`virtual`
- 外部函数：`watchdog_active`
- 外部函数：`watchdog_hrtimer_pretimeout_start`
- 外部函数：`watchdog_hrtimer_pretimeout_stop`
- 外部函数：`watchdog_hw_running`
- 大写宏：`EBUSY`
- 大写宏：`ERR_PTR`
- 大写宏：`HRTIMER_MODE_REL_HARD`
- 大写宏：`IS_ERR_OR_NULL`
- 大写宏：`KTIME_MAX`
- 大写宏：`MISC_MAJOR`
- 大写宏：`WDOG_ACTIVE`
- 大写宏：`WDOG_HW_RUNNING`
- 大写宏：`WDOG_NO_WAY_OUT`
- 外部类型：`All`
- 外部类型：`Alternatively`
- 外部类型：`Failed`
- 外部类型：`Get`
- 外部类型：`If`
- 外部类型：`Reference`
- 外部类型：`The`
- 外部类型：`To`
- 外部类型：`Userspace`
- 外部类型：`dev_t`
- 外部类型：`kthread_worker`
- 外部类型：`ktime_t`
- 外部类型：`watchdog_core_data`
- 外部类型：`watchdog_device`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-f751a2f065` → 本草稿移入 `cases/defect/auto-linux-f751a2f065/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
