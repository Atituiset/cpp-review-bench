# auto-linux-a25e1d0e87

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#9f2516f45b7cfca237f4d8e1c91c1f307e295dc5](https://github.com/torvalds/linux/commit/9f2516f45b7cfca237f4d8e1c91c1f307e295dc5) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-24 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 20 |
| 编译错误数（gcc syntax-only） | 27（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #9f2516f45b7cfca237f4d8e1c91c1f307e295dc5 (https://github.com/torvalds/linux/commit/9f2516f45b7cfca237f4d8e1c91c1f307e295dc5)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 734；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 9f2516f45b7cfca237f4d8e1c91c1f307e295dc5 Merge tag 'for_linus' of git://git.kernel.org/pub/scm/linux/kernel/git/mst/vhost :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -731,7 +731,7 @@ static void vhost_worker_destroy(struct vhost_dev *dev,
 	kfree(worker);
 }
 
-static void vhost_workers_free(struct vhost_dev *dev)
+static void vhost_workers_free(struct vhost_dev *dev, bool sync)
 {
 	struct vhost_worker *worker;
 	unsigned long i;
@@ -741,6 +741,21 @@ static void vhost_workers_free(struct vhost_dev *dev)
 
 	for (i = 0; i < dev->nvqs; i++)
 		rcu_assign_pointer(dev->vqs[i]->worker, NULL);
+
+	/*
+	 * This path can be reached by lockless work queuers.  In this case
+	 * vhost_vq_work_queue() reads vq->worker under rcu_read_lock(), so a
+	 * RCU reader that fetched a worker before we cleared the pointers above
+	 * may still be queueing work on it.  Wait for those readers to finish,
+	 * then flush so any work they queued runs (clearing VHOST_WORK_QUEUED)
+	 * before the workers are freed.  Notably, that is the case for the
+	 * vsock RESET_OWNER path.
+	 */
+	if (sync) {
+		synchronize_rcu();
+		vhost_dev_flush(dev);
+	}
+
 	/*
 	 * Free the default worker we created and cleanup workers userspace
 	 * created but couldn't clean up (it forgot or crashed).
@@ -1163,11 +1178,12 @@ struct vhost_iotlb *vhost_dev_reset_owner_prepare(void)
 EXPORT_SYMBOL_GPL(vhost_dev_reset_owner_prepare);
 
 /* Caller should have device mutex */
-void vhost_dev_reset_owner(struct vhost_dev *dev, struct vhost_iotlb *umem)
+void vhost_dev_reset_owner(struct vhost_dev *dev, struct vhost_iotlb *umem,
+			   bool sync)
 {
 	int i;
 
-	vhost_dev_cleanup(dev);
+	vhost_dev_cleanup(dev, sync);
 
 	dev->fork_owner = fork_from_owner_default;
 	dev->umem = umem;
@@ -1227,7 +1243,7 @@ void vhost_clear_msg(struct vhost_dev *dev)
 }
 EXPORT_SYMBOL_GPL(vhost_clear_msg);
 
-void vhost_dev_cleanup(struct vhost_dev *dev)
+void vhost_dev_cleanup(struct vhost_dev *dev, bool sync)
 {
 	int i;
 
@@ -1251,7 +1267,7 @@ void vhost_dev_cleanup(struct vhost_dev *dev)
 	dev->iotlb = NULL;
 	vhost_clear_msg(dev);
 	wake_up_interruptible_poll(&dev->wait, EPOLLIN | EPOLLRDNORM);
-	vhost_workers_free(dev);
+	vhost_workers_free(dev, sync);
 	vhost_detach_mm(dev);
 }
 EXPORT_SYMBOL_GPL(vhost_dev_cleanup);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`iotlb_alloc`
- 外部函数：`kfree`
- 外部函数：`list_del`
- 外部函数：`mmdrop`
- 外部函数：`mmput`
- 外部函数：`rcu_assign_pointer`
- 外部函数：`spin_lock`
- 外部函数：`spin_unlock`
- 外部函数：`vhost_iotlb_alloc`
- 外部函数：`wake_up_interruptible_poll`
- 大写宏：`EPOLLIN`
- 大写宏：`EPOLLRDNORM`
- 大写宏：`EXPORT_SYMBOL_GPL`
- 大写宏：`NULL`
- 大写宏：`VHOST_IOTLB_FLAG_RETIRE`
- 外部类型：`Caller`
- 外部类型：`Free`
- 外部类型：`vhost_dev`
- 外部类型：`vhost_iotlb`
- 外部类型：`vhost_msg_node`
- 外部类型：`vhost_worker`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-a25e1d0e87` → 本草稿移入 `cases/defect/auto-linux-a25e1d0e87/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
