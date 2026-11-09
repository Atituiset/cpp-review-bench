# auto-linux-3abcb06a97

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#e6f836ef7865fb454641febe080cf88a3c677186](https://github.com/torvalds/linux/commit/e6f836ef7865fb454641febe080cf88a3c677186) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-09 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 94 |
| 编译错误数（gcc syntax-only） | 157（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #e6f836ef7865fb454641febe080cf88a3c677186 (https://github.com/torvalds/linux/commit/e6f836ef7865fb454641febe080cf88a3c677186)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 2081；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT e6f836ef7865fb454641febe080cf88a3c677186 Merge tag 'ata-7.4-rc2' of git://git.kernel.org/pub/scm/linux/kernel/git/libata/linux :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2059,6 +2059,10 @@ void ata_dev_power_set_standby(struct ata_device *dev)
 	struct ata_taskfile tf;
 	unsigned int err_mask;
 
+	/* Issue STANDBY IMMEDIATE command only if supported by the device */
+	if (!ata_dev_power_init_tf(dev, &tf, false))
+		return;
+
 	/* If the device is already sleeping or in standby, do nothing. */
 	if ((dev->flags & ATA_DFLAG_SLEEPING) ||
 	    !ata_dev_power_is_active(dev))
@@ -2077,10 +2081,6 @@ void ata_dev_power_set_standby(struct ata_device *dev)
 	    system_entering_hibernation())
 		return;
 
-	/* Issue STANDBY IMMEDIATE command only if supported by the device */
-	if (!ata_dev_power_init_tf(dev, &tf, false))
-		return;
-
 	ata_dev_notice(dev, "Entering standby power mode\n");
 
 	err_mask = ata_exec_internal(dev, &tf, NULL, DMA_NONE, NULL, 0, 0);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__acquire`
- 外部函数：`__ata_qc_from_tag`
- 外部函数：`__release`
- 外部函数：`ata_dev_dbg`
- 外部函数：`ata_dev_err`
- 外部函数：`ata_dev_notice`
- 外部函数：`ata_dev_power_set_active`
- 外部函数：`ata_dev_warn`
- 外部函数：`ata_eh_acquire`
- 外部函数：`ata_eh_release`
- 外部函数：`ata_internal_cmd_timed_out`
- 外部函数：`ata_internal_cmd_timeout`
- 外部函数：`ata_port_freeze`
- 外部函数：`ata_port_is_frozen`
- 外部函数：`ata_qc_issue`
- 外部函数：`ata_qc_reinit`
- 外部函数：`ata_sff_flush_pio_task`
- 外部函数：`ata_tag_valid`
- 外部函数：`ata_tf_init`
- 外部函数：`complete`
- 外部函数：`failed`
- 外部函数：`memcpy`
- 外部函数：`msecs`
- 外部函数：`msecs_to_jiffies`
- 外部函数：`post_internal_cmd`
- 外部函数：`sg_init_one`
- 外部函数：`spin_lock_irqsave`
- 外部函数：`spin_unlock_irqrestore`
- 外部函数：`system_entering_hibernation`
- 外部函数：`wait_for_completion_timeout`
- 大写宏：`AC_ERR_DEV`
- 大写宏：`AC_ERR_INVALID`
- 大写宏：`AC_ERR_OTHER`
- 大写宏：`AC_ERR_SYSTEM`
- 大写宏：`AC_ERR_TIMEOUT`
- 大写宏：`ATA`
- 大写宏：`ATAPI_CDB_LEN`
- 大写宏：`ATAPI_DMADIR`
- 大写宏：`ATAPI_PROT_DMA`
- 大写宏：`ATA_CMD_CHK_POWER`
- 大写宏：`ATA_CMD_REQ_SENSE_DATA`
- 大写宏：`ATA_CMD_STANDBYNOW1`
- 大写宏：`ATA_CMD_VERIFY`
- 大写宏：`ATA_DEV_ATA`
- 大写宏：`ATA_DEV_ZAC`
- 大写宏：`ATA_DF`
- 大写宏：`ATA_DFLAG_DMADIR`
- 大写宏：`ATA_DFLAG_LBA`
- 大写宏：`ATA_DFLAG_SLEEPING`
- 大写宏：`ATA_ERR`
- 大写宏：`ATA_LBA`
- 大写宏：`ATA_PROT_NODATA`
- 大写宏：`ATA_QCFLAG_ACTIVE`
- 大写宏：`ATA_QCFLAG_EH`
- 大写宏：`ATA_QCFLAG_RESULT_TF`
- 大写宏：`ATA_SENSE`
- 大写宏：`ATA_TAG_INTERNAL`
- 大写宏：`ATA_TAG_POISON`
- 大写宏：`ATA_TFLAG_DEVICE`
- 大写宏：`ATA_TFLAG_ISADDR`
- 大写宏：`ATA_TFLAG_LBA`
- 大写宏：`CHS`
- 大写宏：`DECLARE_COMPLETION_ONSTACK`
- 大写宏：`DMA_FROM_DEVICE`
- 大写宏：`DMA_NONE`
- 大写宏：`IMMEDIATE`
- 大写宏：`NULL`
- 大写宏：`SATA`
- 大写宏：`STANDBY`
- 大写宏：`VERIFY`
- 大写宏：`WARN_ON`
- 大写宏：`ZAC`
- 外部类型：`Active`
- 外部类型：`Assume`
- 外部类型：`Check`
- 外部类型：`Entering`
- 外部类型：`Finish`
- 外部类型：`If`
- 外部类型：`Initialize`
- 外部类型：`Issue`
- 外部类型：`No`
- 外部类型：`Only`
- 外部类型：`Perform`
- 外部类型：`Power`
- 外部类型：`Prepare`
- 外部类型：`Some`
- 外部类型：`To`
- 外部类型：`We`
- 外部类型：`ata_device`
- 外部类型：`ata_link`
- 外部类型：`ata_port`
- 外部类型：`ata_queued_cmd`
- 外部类型：`ata_taskfile`
- 外部类型：`completion`
- 外部类型：`dma_data_direction`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-3abcb06a97` → 本草稿移入 `cases/defect/auto-linux-3abcb06a97/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
