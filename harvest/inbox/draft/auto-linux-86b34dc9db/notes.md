# auto-linux-86b34dc9db

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#e93fd809fb5692d1571bd008988ba734fd7fc976](https://github.com/torvalds/linux/commit/e93fd809fb5692d1571bd008988ba734fd7fc976) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-14 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 147 |
| 编译错误数（gcc syntax-only） | 158（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #e93fd809fb5692d1571bd008988ba734fd7fc976 (https://github.com/torvalds/linux/commit/e93fd809fb5692d1571bd008988ba734fd7fc976)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 5032；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT e93fd809fb5692d1571bd008988ba734fd7fc976 Merge tag 'block-7.4-20261112' of git://git.kernel.org/pub/scm/linux/kernel/git/axboe/linux :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -881,7 +881,8 @@ static ssize_t ublk_batch_copy_io_tags(struct ublk_batch_fetch_cmd *fcmd,
  * by cgroup.
  */
 static unsigned int unprivileged_ublks_max = 64;
-static unsigned int unprivileged_ublks_added; /* protected by ublk_ctl_mutex */
+/* checked and incremented under ublk_ctl_mutex, decremented without it */
+static atomic_t unprivileged_ublks_added = ATOMIC_INIT(0);
 
 static struct miscdevice ublk_misc;
 
@@ -4405,14 +4406,18 @@ static int ublk_alloc_dev_number(struct ublk_device *ub, int idx)
 	int i = idx;
 	int err;
 
+	/*
+	 * Only reserve the id: the device is published by
+	 * ublk_publish_dev_number() once it is fully set up.
+	 */
 	spin_lock(&ublk_idr_lock);
 	/* allocate id, if @id >= 0, we're requesting that specific id */
 	if (i >= 0) {
-		err = idr_alloc(&ublk_index_idr, ub, i, i + 1, GFP_NOWAIT);
+		err = idr_alloc(&ublk_index_idr, NULL, i, i + 1, GFP_NOWAIT);
 		if (err == -ENOSPC)
 			err = -EEXIST;
 	} else {
-		err = idr_alloc(&ublk_index_idr, ub, 0, UBLK_MAX_UBLKS,
+		err = idr_alloc(&ublk_index_idr, NULL, 0, UBLK_MAX_UBLKS,
 				GFP_NOWAIT);
 	}
 	spin_unlock(&ublk_idr_lock);
@@ -4423,6 +4428,19 @@ static int ublk_alloc_dev_number(struct ublk_device *ub, int idx)
 	return err;
 }
 
+/*
+ * Make the device visible to control commands by its id. Before its char
+ * device is added, DEL_DEV could find it and remove a char device that was
+ * never added, and drop the reference that ublk_add_chdev() drops on
+ * failure.
+ */
+static void ublk_publish_dev_number(struct ublk_device *ub)
+{
+	spin_lock(&ublk_idr_lock);
+	WARN_ON_ONCE(idr_replace(&ublk_index_idr, ub, ub->ub_number) != NULL);
+	spin_unlock(&ublk_idr_lock);
+}
+
 static void ublk_free_dev_number(struct ublk_device *ub)
 {
 	spin_lock(&ublk_idr_lock);
@@ -4469,7 +4487,7 @@ static int ublk_add_chdev(struct ublk_device *ub)
 		goto fail;
 
 	if (ub->dev_info.flags & UBLK_F_UNPRIVILEGED_DEV)
-		unprivileged_ublks_added++;
+		atomic_inc(&unprivileged_ublks_added);
 	return 0;
  fail:
 	put_device(dev);
@@ -4501,15 +4519,9 @@ static int ublk_add_tag_set(struct ublk_device *ub)
 
 static void ublk_remove(struct ublk_device *ub)
 {
-	bool unprivileged;
-
 	ublk_stop_dev(ub);
 	cdev_device_del(&ub->cdev, &ub->cdev_dev);
-	unprivileged = ub->dev_info.flags & UBLK_F_UNPRIVILEGED_DEV;
 	ublk_put_device(ub);
-
-	if (unprivileged)
-		unprivileged_ublks_added--;
 }
 
 static struct ublk_device *ublk_get_device_from_id(int idx)
@@ -4899,7 +4911,7 @@ static int ublk_ctrl_add_dev(const struct ublksrv_ctrl_cmd *header)
 
 	ret = -EACCES;
 	if ((info.flags & UBLK_F_UNPRIVILEGED_DEV) &&
-	    unprivileged_ublks_added >= unprivileged_ublks_max)
+	    atomic_read(&unprivileged_ublks_added) >= unprivileged_ublks_max)
 		goto out_unlock;
 
 	ret = -ENOMEM;
@@ -4991,6 +5003,8 @@ static int ublk_ctrl_add_dev(const struct ublksrv_ctrl_cmd *header)
 	 * ublk_add_chdev() will cleanup everything if it fails.
 	 */
 	ret = ublk_add_chdev(ub);
+	if (!ret)
+		ublk_publish_dev_number(ub);
 	goto out_unlock;
 
 out_deinit_queues:
@@ -5023,23 +5037,23 @@ static int ublk_ctrl_del_dev(struct ublk_device **p_ub, bool wait)
 {
 	struct ublk_device *ub = *p_ub;
 	int idx = ub->ub_number;
-	int ret;
-
-	ret = mutex_lock_killable(&ublk_ctl_mutex);
-	if (ret)
-		return ret;
+	bool unprivileged = ub->dev_info.flags & UBLK_F_UNPRIVILEGED_DEV;
 
-	if (!test_bit(UB_STATE_DELETED, &ub->state)) {
+	/*
+	 * Don't take ublk_ctl_mutex: stopping the device may wait for the
+	 * filesystem on its disk indefinitely, and ADD_DEV and DEL_DEV of all
+	 * other ublk devices would wait for that too. ADD_DEV is done with the
+	 * device, it is found by its id only then.
+	 */
+	if (!test_and_set_bit(UB_STATE_DELETED, &ub->state)) {
 		ublk_remove(ub);
-		set_bit(UB_STATE_DELETED, &ub->state);
+		if (unprivileged)
+			atomic_dec(&unprivileged_ublks_added);
 	}
 
-	mutex_unlock(&ublk_ctl_mutex);
-
 	/*
-	 * Drop the reference outside ublk_ctl_mutex: if it is the last one,
-	 * the release frees the 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__counted_by`
- 外部函数：`__fput_sync`
- 外部函数：`__ublk_abort_rq`
- 外部函数：`_iter`
- 外部函数：`blk_mq_free_tag_set`
- 外部函数：`blk_mq_request_started`
- 外部函数：`blk_mq_tag_to_rq`
- 外部函数：`cancel_work_sync`
- 外部函数：`cdev_device_add`
- 外部函数：`cdev_device_del`
- 外部函数：`cdev_init`
- 外部函数：`command`
- 外部函数：`cond_resched`
- 外部函数：`container_of`
- 外部函数：`dev_set_name`
- 外部函数：`device_initialize`
- 外部函数：`file_ref_get`
- 外部函数：`free_pages`
- 外部函数：`get_order`
- 外部函数：`ida_destroy`
- 外部函数：`idr_alloc`
- 外部函数：`idr_find`
- 外部函数：`io_uring_cmd_complete_in_task`
- 外部函数：`io_uring_cmd_done`
- 外部函数：`kfree`
- 外部函数：`kobject_get_unless_zero`
- 外部函数：`kvfree`
- 外部函数：`list_del_init`
- 外部函数：`list_empty`
- 外部函数：`list_first_entry`
- 外部函数：`list_move`
- 外部函数：`list_splice_init`
- 外部函数：`mas_erase`
- 外部函数：`mas_lock`
- 外部函数：`mas_unlock`
- 外部函数：`mtree_destroy`
- 外部函数：`mutex_destroy`
- 外部函数：`mutex_lock`
- 外部函数：`mutex_lock_killable`
- 外部函数：`mutex_unlock`
- 外部函数：`put_device`
- 外部函数：`put_task_struct`
- 外部函数：`refcount_read`
- 外部函数：`set_bit`
- 外部函数：`smp_mb__after_atomic`
- 外部函数：`smp_wmb`
- 外部函数：`spin_lock`
- 外部函数：`spin_unlock`
- 外部函数：`test_bit`
- 外部函数：`time`
- 外部函数：`ublk_batch_free_fcmd`
- 外部函数：`ublk_dispatch_req`
- 外部函数：`ublk_io_evts_deinit`
- 外部函数：`ublk_prep_cancel`
- 外部函数：`ublk_queue_cmd_buf_size`
- 外部函数：`ublk_queue_rq`
- 外部函数：`ublk_queue_rqs`
- 外部函数：`ublk_reset_ch_dev`
- 外部函数：`ublk_set_canceling`
- 外部函数：`ublk_stop_dev_unlocked`
- 外部函数：`ublk_unpin_range_pages`
- 外部函数：`wake_up_bit`
- 外部函数：`wake_up_var`
- 大写宏：`ACTIVE`
- 大写宏：`DECLARE_KFIFO_PTR`
- 大写宏：`EACCES`
- 大写宏：`EEXIST`
- 大写宏：`ENOENT`
- 大写宏：`ENOMEM`
- 大写宏：`ENOSPC`
- 大写宏：`FETCH`
- 大写宏：`GFP_NOWAIT`
- 大写宏：`IDLE`
- 大写宏：`IO_URING_F_UNLOCKED`
- 大写宏：`LIST_HEAD`
- 大写宏：`MAJOR`
- 大写宏：`MA_STATE`
- 大写宏：`MINORBITS`
- 大写宏：`MKDEV`
- 大写宏：`NULL`
- 大写宏：`PREP`
- 大写宏：`READY`
- 大写宏：`READ_ONCE`
- 大写宏：`SRCU`
- 大写宏：`START_DEV`
- 大写宏：`STOPPING`
- 大写宏：`STOP_DEV`
- 大写宏：`UBLK_F_BATCH_IO`
- 大写宏：`UBLK_F_UNPRIVILEGED_DEV`
- 大写宏：`UBLK_IO_COMMIT_AND_FETCH_REQ`
- 大写宏：`UBLK_IO_FLAG_OWNED_BY_SRV`
- 大写宏：`UBLK_IO_RES_ABORT`
- 大写宏：`UBLK_REFCOUNT_INIT`
- 大写宏：`UBLK_REMOVE_BATCH`
- 大写宏：`UBLK_S_DEV_FAIL_IO`
- 大写宏：`ULONG_MAX`
- 大写宏：`WARN_ON_ONCE`
- 外部类型：`All`
- 外部类型：`At`
- 外部类型：`Batch`
- 外部类型：`Count`
- 外部类型：`Currently`
- 外部类型：`Don`
- 外部类型：`Drop`
- 外部类型：`Either`
- 外部类型：`Fetch`
- 外部类型：`For`
- 外部类型：`If`
- 外部类型：`Inflight`
- 外部类型：`Invariants`
- 外部类型：`Key`
- 外部类型：`List`
- 外部类型：`Make`
- 外部类型：`Management`
- 外部类型：`Multiple`
- 外部类型：`No`
- 外部类型：`One`
- 外部类型：`Set`
- 外部类型：`So`
- 外部类型：`State`
- 外部类型：`The`
- 外部类型：`There`
- 外部类型：`User`
- 外部类型：`While`
- 外部类型：`blk_mq_tag_set`
- 外部类型：`cdev`
- 外部类型：`delayed_work`
- 外部类型：`device`
- 外部类型：`file`
- 外部类型：`gendisk`
- 外部类型：`ida`
- 外部类型：`io_uring_cmd`
- 外部类型：`list_head`
- 外部类型：`maple_tree`
- 外部类型：`miscdevice`
- 外部类型：`mm_struct`
- 外部类型：`mutex`
- 外部类型：`pid_t`
- 外部类型：`refcount_t`
- 外部类型：`request`
- 外部类型：`size_t`
- 外部类型：`spinlock_t`
- 外部类型：`task_struct`
- 外部类型：`ublk_auto_buf_reg`
- 外部类型：`ublk_buf_range`
- 外部类型：`ublk_params`
- 外部类型：`ublk_unpin_range`
- 外部类型：`ublksrv_ctrl_dev_info`
- 外部类型：`ublksrv_io_desc`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-86b34dc9db` → 本草稿移入 `cases/defect/auto-linux-86b34dc9db/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
