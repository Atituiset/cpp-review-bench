# auto-linux-95dedc726d

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#c44104f3642da720164042623e4832836aefdd8e](https://github.com/torvalds/linux/commit/c44104f3642da720164042623e4832836aefdd8e) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-21 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 72 |
| 编译错误数（gcc syntax-only） | 110（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #c44104f3642da720164042623e4832836aefdd8e (https://github.com/torvalds/linux/commit/c44104f3642da720164042623e4832836aefdd8e)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 153；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT c44104f3642da720164042623e4832836aefdd8e Merge tag 'drm-fixes-2026-11-21' of https://gitlab.freedesktop.org/drm/kernel :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -150,8 +150,8 @@ static void ivpu_pm_reset_complete(struct ivpu_device *vdev)
 	up_write(&vdev->pm->reset_lock);
 	atomic_set(&vdev->pm->reset_pending, 0);
 
-	pm_runtime_mark_last_busy(vdev->drm.dev);
 	pm_runtime_enable(vdev->drm.dev);
+	pm_request_autosuspend(vdev->drm.dev);
 }
 
 static void ivpu_pm_recovery_work(struct work_struct *work)
@@ -415,6 +415,7 @@ void ivpu_pm_init(struct ivpu_device *vdev)
 	atomic_set(&pm->engine_reset_counter, 0);
 
 	INIT_WORK(&pm->recovery_work, ivpu_pm_recovery_work);
+	disable_work(&pm->recovery_work);
 	INIT_DELAYED_WORK(&pm->job_timeout_work, ivpu_job_timeout_work);
 
 	if (ivpu_disable_recovery)
@@ -439,6 +440,12 @@ void ivpu_pm_enable(struct ivpu_device *vdev)
 {
 	struct device *dev = vdev->drm.dev;
 
+	enable_work(&vdev->pm->recovery_work);
+	if (atomic_read(&vdev->pm->reset_pending)) {
+		queue_work(system_dfl_wq, &vdev->pm->recovery_work);
+		flush_work(&vdev->pm->recovery_work);
+	}
+
 	pm_runtime_allow(dev);
 	pm_runtime_put_autosuspend(dev);
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`atomic_cmpxchg`
- 外部函数：`atomic_fetch_inc`
- 外部函数：`atomic_inc`
- 外部函数：`atomic_read`
- 外部函数：`atomic_set`
- 外部函数：`container_of`
- 外部函数：`down_write`
- 外部函数：`ivpu_boot`
- 外部函数：`ivpu_cmdq_reset_all_contexts`
- 外部函数：`ivpu_dbg`
- 外部函数：`ivpu_dev_coredump`
- 外部函数：`ivpu_err`
- 外部函数：`ivpu_fw_is_warm_boot`
- 外部函数：`ivpu_fw_log_reset`
- 外部函数：`ivpu_hw_diagnose_failure`
- 外部函数：`ivpu_hw_irq_disable`
- 外部函数：`ivpu_hw_power_down`
- 外部函数：`ivpu_hw_power_up`
- 外部函数：`ivpu_ipc_reset`
- 外部函数：`ivpu_jobs_abort_all`
- 外部函数：`ivpu_jsm_get_heartbeat`
- 外部函数：`ivpu_jsm_state_dump`
- 外部函数：`ivpu_jsm_state_dump_no_reply`
- 外部函数：`ivpu_mmu_disable`
- 外部函数：`ivpu_mmu_enable`
- 外部函数：`ivpu_ms_cleanup_all`
- 外部函数：`ivpu_prepare_for_reset`
- 外部函数：`ivpu_shutdown`
- 外部函数：`kobject_uevent_env`
- 外部函数：`limit`
- 外部函数：`msecs_to_jiffies`
- 外部函数：`pci_restore_state`
- 外部函数：`pci_set_power_state`
- 外部函数：`pm_runtime_allow`
- 外部函数：`pm_runtime_disable`
- 外部函数：`pm_runtime_enable`
- 外部函数：`pm_runtime_mark_last_busy`
- 外部函数：`pm_runtime_put_autosuspend`
- 外部函数：`pm_runtime_set_active`
- 外部函数：`pm_runtime_set_suspended`
- 外部函数：`pm_runtime_status_suspended`
- 外部函数：`queue_delayed_work`
- 外部函数：`queue_work`
- 外部函数：`to_pci_dev`
- 外部函数：`up_write`
- 大写宏：`DIV_ROUND_UP`
- 大写宏：`FW_BOOT`
- 大写宏：`INIT_DELAYED_WORK`
- 大写宏：`INIT_WORK`
- 大写宏：`IRQ`
- 大写宏：`IVPU_PM_EVENT`
- 大写宏：`IVPU_RECOVER`
- 大写宏：`KOBJ_CHANGE`
- 大写宏：`MMU`
- 大写宏：`NPU`
- 大写宏：`NULL`
- 大写宏：`PCI_D0`
- 大写宏：`VPU_BOOT_TYPE_COLDBOOT`
- 大写宏：`VPU_SCHEDULING_MODE_OS`
- 外部类型：`Cold`
- 外部类型：`Disable`
- 外部类型：`Failed`
- 外部类型：`Job`
- 外部类型：`No`
- 外部类型：`PCI_D3hot`
- 外部类型：`Recovering`
- 外部类型：`Recovery`
- 外部类型：`Trigger`
- 外部类型：`device`
- 外部类型：`ivpu_device`
- 外部类型：`ivpu_fw_info`
- 外部类型：`ivpu_pm_info`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-95dedc726d` → 本草稿移入 `cases/defect/auto-linux-95dedc726d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
