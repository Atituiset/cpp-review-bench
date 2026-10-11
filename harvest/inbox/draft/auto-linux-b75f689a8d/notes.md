# auto-linux-b75f689a8d

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#c61e23ed14359e6289c08e55940dd3401fe27db0](https://github.com/torvalds/linux/commit/c61e23ed14359e6289c08e55940dd3401fe27db0) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-11 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 32 |
| 编译错误数（gcc syntax-only） | 27（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #c61e23ed14359e6289c08e55940dd3401fe27db0 (https://github.com/torvalds/linux/commit/c61e23ed14359e6289c08e55940dd3401fe27db0)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 184；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT c61e23ed14359e6289c08e55940dd3401fe27db0 Merge tag 'drm-misc-fixes-2026-10-09' of https://gitlab.freedesktop.org/drm/misc/kernel into drm-fixes :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -173,46 +173,42 @@ static void vc4_v3d_init_hw(struct drm_device *dev)
 	V3D_WRITE(V3D_VPMBASE, 0);
 }
 
-int vc4_v3d_get_bin_slot(struct vc4_dev *vc4)
+static int bin_slot_try_alloc(struct vc4_dev *vc4)
 {
-	struct drm_device *dev = &vc4->base;
-	unsigned long irqflags;
 	int slot;
-	uint64_t seqno = 0;
-	struct vc4_exec_info *exec;
 
-	if (WARN_ON_ONCE(vc4->gen > VC4_GEN_4))
-		return -ENODEV;
+	guard(spinlock_irqsave)(&vc4->job_lock);
 
-try_again:
-	spin_lock_irqsave(&vc4->job_lock, irqflags);
 	slot = ffs(~vc4->bin_alloc_used);
 	if (slot != 0) {
 		/* Switch from ffs() bit index to a 0-based index. */
 		slot--;
 		vc4->bin_alloc_used |= BIT(slot);
-		spin_unlock_irqrestore(&vc4->job_lock, irqflags);
-		return slot;
+	} else {
+		slot = -ENOMEM;
 	}
 
-	/* Couldn't find an open slot.  Wait for render to complete
-	 * and try again.
-	 */
-	exec = vc4_last_render_job(vc4);
-	if (exec)
-		seqno = exec->seqno;
-	spin_unlock_irqrestore(&vc4->job_lock, irqflags);
+	return slot;
+}
 
-	if (seqno) {
-		int ret = vc4_wait_for_seqno(dev, seqno, ~0ull, true);
+int vc4_v3d_get_bin_slot(struct vc4_dev *vc4, long timeout)
+{
+	int slot;
+	long ret;
 
-		if (ret == 0)
-			goto try_again;
+	if (WARN_ON_ONCE(vc4->gen > VC4_GEN_4))
+		return -ENODEV;
 
+	/* If the pool is full, wait for a job to release its slots. */
+	ret = wait_event_interruptible_timeout(vc4->job_wait_queue,
+					       (slot = bin_slot_try_alloc(vc4)) >= 0,
+					       timeout);
+	if (ret < 0)
 		return ret;
-	}
+	if (ret == 0)
+		return -ENOMEM;
 
-	return -ENOMEM;
+	return slot;
 }
 
 /*
@@ -397,6 +393,7 @@ static int vc4_v3d_runtime_resume(struct device *dev)
 	vc4_v3d_init_hw(&vc4->base);
 
 	vc4_irq_enable(&vc4->base);
+	enable_irq(vc4->irq);
 
 	return 0;
 }
@@ -453,6 +450,12 @@ static int vc4_v3d_bind(struct device *dev, struct device *master, void *data)
 		return ret;
 	vc4->irq = ret;
 
+	ret = vc4_irq_install(drm, vc4->irq);
+	if (ret) {
+		drm_err(drm, "Failed to install IRQ handler\n");
+		return ret;
+	}
+
 	ret = devm_pm_runtime_enable(dev);
 	if (ret)
 		return ret;
@@ -474,12 +477,6 @@ static int vc4_v3d_bind(struct device *dev, struct device *master, void *data)
 	V3D_WRITE(V3D_BPOA, 0);
 	V3D_WRITE(V3D_BPOS, 0);
 
-	ret = vc4_irq_install(drm, vc4->irq);
-	if (ret) {
-		drm_err(drm, "Failed to install IRQ handler\n");
-		goto err_put_runtime_pm;
-	}
-
 	pm_runtime_use_autosuspend(dev);
 	pm_runtime_set_autosuspend_delay(dev, 40); /* a little over 2 frames. */
 	pm_runtime_put_autosuspend(dev);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`devm_pm_runtime_enable`
- 外部函数：`drm_err`
- 外部函数：`ffs`
- 外部函数：`pm_runtime_put_autosuspend`
- 外部函数：`pm_runtime_set_autosuspend_delay`
- 外部函数：`pm_runtime_use_autosuspend`
- 外部函数：`spin_lock_irqsave`
- 外部函数：`spin_unlock_irqrestore`
- 外部函数：`to_vc4_dev`
- 外部函数：`vc4_irq_enable`
- 外部函数：`vc4_irq_install`
- 外部函数：`vc4_last_render_job`
- 外部函数：`vc4_wait_for_seqno`
- 大写宏：`BIT`
- 大写宏：`ENODEV`
- 大写宏：`ENOMEM`
- 大写宏：`IRQ`
- 大写宏：`QPU`
- 大写宏：`V3D_BPOA`
- 大写宏：`V3D_BPOS`
- 大写宏：`V3D_VPMBASE`
- 大写宏：`V3D_WRITE`
- 大写宏：`VC4_GEN_4`
- 大写宏：`WARN_ON_ONCE`
- 外部类型：`Couldn`
- 外部类型：`Failed`
- 外部类型：`Switch`
- 外部类型：`Take`
- 外部类型：`Wait`
- 外部类型：`drm_device`
- 外部类型：`uint64_t`
- 外部类型：`vc4_dev`
- 外部类型：`vc4_exec_info`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-b75f689a8d` → 本草稿移入 `cases/defect/auto-linux-b75f689a8d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
