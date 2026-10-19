# auto-linux-ed7b209a13

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#f909e534f5e34feb2ea2701d54df63e21aa506fc](https://github.com/torvalds/linux/commit/f909e534f5e34feb2ea2701d54df63e21aa506fc) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-19 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 36 |
| 编译错误数（gcc syntax-only） | 59（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #f909e534f5e34feb2ea2701d54df63e21aa506fc (https://github.com/torvalds/linux/commit/f909e534f5e34feb2ea2701d54df63e21aa506fc)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 65；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT f909e534f5e34feb2ea2701d54df63e21aa506fc Merge tag 'scsi-fixes' of git://git.kernel.org/pub/scm/linux/kernel/git/jejb/scsi :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -62,7 +62,11 @@ static int smp_execute_task_sg(struct domain_device *dev,
 		to_sas_internal(dev->port->ha->shost->transportt);
 	struct sas_ha_struct *ha = dev->port->ha;
 
-	pm_runtime_get_sync(ha->dev);
+	/*
+	 * Non-blocking: a sync resume here would deadlock against
+	 * sas_drain_work() during HA resume.
+	 */
+	pm_runtime_get_noresume(ha->dev);
 	mutex_lock(&dev->ex_dev.cmd_mutex);
 	for (retry = 0; retry < 3; retry++) {
 		if (test_bit(SAS_DEV_GONE, &dev->state)) {
@@ -135,7 +139,7 @@ static int smp_execute_task_sg(struct domain_device *dev,
 		}
 	}
 	mutex_unlock(&dev->ex_dev.cmd_mutex);
-	pm_runtime_put_sync(ha->dev);
+	pm_runtime_put(ha->dev);
 
 	BUG_ON(retry == 3 && task != NULL);
 	sas_free_task(task);
@@ -2222,13 +2226,19 @@ void sas_smp_handler(struct bsg_job *job, struct Scsi_Host *shost,
 		goto out;
 	}
 
+	/* The host may have autosuspended, resume it here. */
+	ret = pm_runtime_resume_and_get(dev->port->ha->dev);
+	if (ret)
+		goto out;
+
 	ret = smp_execute_task_sg(dev, job->request_payload.sg_list,
 			job->reply_payload.sg_list);
 	if (ret >= 0) {
 		/* bsg_job_done() requires the length received  */
 		rcvlen = job->reply_payload.payload_len - ret;
 		ret = 0;
 	}
+	pm_runtime_put(dev->port->ha->dev);
 
 out:
 	bsg_job_done(job, ret, rcvlen);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`add_timer`
- 外部函数：`bsg_job_done`
- 外部函数：`lldd_abort_task`
- 外部函数：`lldd_execute_task`
- 外部函数：`mutex_lock`
- 外部函数：`mutex_unlock`
- 外部函数：`pm_runtime_get_sync`
- 外部函数：`pm_runtime_put_sync`
- 外部函数：`pr_notice`
- 外部函数：`sas_alloc_slow_task`
- 外部函数：`sas_free_task`
- 外部函数：`test_bit`
- 外部函数：`timer_delete_sync`
- 外部函数：`to_sas_internal`
- 外部函数：`wait_for_completion`
- 大写宏：`BUG_ON`
- 大写宏：`ECOMM`
- 大写宏：`EMSGSIZE`
- 大写宏：`ENOMEM`
- 大写宏：`GFP_KERNEL`
- 大写宏：`NULL`
- 大写宏：`SAS_ADDR`
- 大写宏：`SAS_DATA_OVERRUN`
- 大写宏：`SAS_DATA_UNDERRUN`
- 大写宏：`SAS_DEVICE_UNKNOWN`
- 大写宏：`SAS_DEV_GONE`
- 大写宏：`SAS_SAM_STAT_GOOD`
- 大写宏：`SAS_TASK_COMPLETE`
- 大写宏：`SAS_TASK_STATE_ABORTED`
- 大写宏：`SAS_TASK_STATE_DONE`
- 大写宏：`SAS_TASK_UNDELIVERED`
- 大写宏：`SMP`
- 外部类型：`domain_device`
- 外部类型：`sas_ha_struct`
- 外部类型：`sas_internal`
- 外部类型：`sas_task`
- 外部类型：`scatterlist`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-ed7b209a13` → 本草稿移入 `cases/defect/auto-linux-ed7b209a13/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
