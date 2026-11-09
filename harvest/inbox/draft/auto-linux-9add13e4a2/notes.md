# auto-linux-9add13e4a2

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
| 外部依赖数（dep_count） | 161 |
| 编译错误数（gcc syntax-only） | 101（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #e6f836ef7865fb454641febe080cf88a3c677186 (https://github.com/torvalds/linux/commit/e6f836ef7865fb454641febe080cf88a3c677186)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 4699；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT e6f836ef7865fb454641febe080cf88a3c677186 Merge tag 'ata-7.4-rc2' of git://git.kernel.org/pub/scm/linux/kernel/git/libata/linux :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2048,6 +2048,19 @@ static int ata_scsi_translate(struct ata_device *dev, struct scsi_cmnd *cmd,
 	if (xlat_func(qc))
 		goto done;
 
+	/*
+	 * A data-out request may contain pages pinned without write access.
+	 * Reject a translated data-in command before issuing it, since its
+	 * completion handler may write to the buffer even if the device fails.
+	 */
+	if (unlikely(cmd->sc_data_direction == DMA_TO_DEVICE &&
+		     ata_is_data(qc->tf.protocol) &&
+		     !(qc->tf.flags & ATA_TFLAG_WRITE))) {
+		ata_scsi_set_sense(dev, cmd, ABORTED_COMMAND,
+				   DATA_IN_BUFFER_ERROR);
+		goto done;
+	}
+
 	return ata_scsi_qc_issue(ap, qc);
 
 done:
@@ -2079,6 +2092,13 @@ static void ata_scsi_rbuf_fill(struct ata_device *dev, struct scsi_cmnd *cmd,
 	unsigned long flags;
 	unsigned int len;
 
+	/* A data-out request may contain pages pinned without write access. */
+	if (cmd->sc_data_direction == DMA_TO_DEVICE) {
+		ata_scsi_set_sense(dev, cmd, ABORTED_COMMAND,
+				   DATA_IN_BUFFER_ERROR);
+		return;
+	}
+
 	spin_lock_irqsave(&ata_scsi_rbuf_lock, flags);
 
 	memset(ata_scsi_rbuf, 0, ATA_SCSI_RBUF_SIZE);
@@ -3173,6 +3193,14 @@ static unsigned int atapi_xlat(struct ata_queued_cmd *qc)
 
 	qc->complete_fn = atapi_qc_complete;
 
+	/* atapi_fixup_inquiry() writes to the buffer on successful completion. */
+	if (scmd->sc_data_direction == DMA_TO_DEVICE &&
+	    scmd->cmnd[0] == INQUIRY) {
+		ata_scsi_set_sense(dev, scmd, ABORTED_COMMAND,
+				   DATA_IN_BUFFER_ERROR);
+		return 1;
+	}
+
 	qc->tf.flags |= ATA_TFLAG_ISADDR | ATA_TFLAG_DEVICE;
 	if (scmd->sc_data_direction == DMA_TO_DEVICE) {
 		qc->tf.flags |= ATA_TFLAG_WRITE;
@@ -4366,14 +4394,22 @@ static unsigned int ata_scsi_zbc_out_xlat(struct ata_queued_cmd *qc)
 /**
  *	ata_mselect_caching - Simulate MODE SELECT for caching info page
  *	@qc: Storage for translated ATA taskfile
- *	@buf: input buffer
- *	@len: number of valid bytes in the input buffer
- *	@fp: out parameter for the failed field on error
+ *	@buf: Mode page, starting with the mode page header
+ *	@len: Length of the mode page, including the mode page header
+ *	@fp: Out parameter, offset in @buf of the first invalid byte on error
  *
  *	Prepare a taskfile to modify caching information for the device.
  *
+ *	The caller has stripped the mode parameter header and the block
+ *	descriptors, so @buf points at the mode page header, like the buffer
+ *	filled by the ata_msense_*() functions, and offsets in @buf, including
+ *	the one returned in @fp, are the byte numbers of the mode page.
+ *
  *	LOCKING:
  *	None.
+ *
+ *	RETURNS:
+ *	0 if a taskfile was prepared, -EINVAL if the mode page is invalid.
  */
 static int ata_mselect_caching(struct ata_queued_cmd *qc,
 			       const u8 *buf, int len, u16 *fp)
@@ -4384,26 +4420,19 @@ static int ata_mselect_caching(struct ata_queued_cmd *qc,
 	u8 wce;
 	int i;
 
-	/*
-	 * The first two bytes of def_cache_mpage are a header, so offsets
-	 * in mpage are off by 2 compared to buf.  Same for len.
-	 */
-
-	if (len != CACHE_MPAGE_LEN - 2) {
-		*fp = min(len, CACHE_MPAGE_LEN - 2);
+	if (len != CACHE_MPAGE_LEN) {
+		*fp = 1;
 		return -EINVAL;
 	}
 
-	wce = buf[0] & (1 << 2);
+	wce = buf[2] & (1 << 2);
 
 	/*
 	 * Check that read-only bits are not modified.
 	 */
 	ata_msense_caching(dev->id, mpage, false);
-	for (i = 0; i < CACHE_MPAGE_LEN - 2; i++) {
-		if (i == 0)
-			continue;
-		if (mpage[i + 2] != buf[i]) {
+	for (i = 3; i < CACHE_MPAGE_LEN; i++) {
+		if (mpage[i] != buf[i]) {
 			*fp = i;
 			return -EINVAL;
 		}
@@ -4417,8 +4446,26 @@ static int ata_mselect_caching(struct ata_queued_cmd *qc,
 	return 0;
 }
 
-/*
- * Simulate MODE SELECT control mode page, sub-page 0.
+/**
+ *	ata_mselect_control_spg0 - Simulate MODE SELECT for control sub-page 0
+ *	@qc: Storage for translated ATA taskfile
+ *	@buf: Mode page, starting with the mode page header
+ *	@len: Length of the mode page, including the mode page header
+ *	@fp: Out parameter, offset in @buf of the first invalid byte 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`ata_dev_dbg`
- 外部函数：`ata_dev_err`
- 外部函数：`ata_eh_set_pending`
- 外部函数：`ata_id_rahead_enabled`
- 外部函数：`ata_id_wcache_enabled`
- 外部函数：`ata_mselect_control_ata_feature`
- 外部函数：`ata_port_eh_scheduled`
- 外部函数：`ata_qc_free`
- 外部函数：`ata_scsi_cmd_error_handler`
- 外部函数：`ata_scsi_qc_issue`
- 外部函数：`cancel_work`
- 外部函数：`data`
- 外部函数：`done`
- 外部函数：`get_host_byte`
- 外部函数：`get_unaligned_be16`
- 外部函数：`lockdep_assert_held`
- 外部函数：`memcpy`
- 外部函数：`memset`
- 外部函数：`min`
- 外部函数：`qc_defer`
- 外部函数：`queue_work`
- 外部函数：`scsi_check_sense`
- 外部函数：`scsi_eh_scmd_add`
- 外部函数：`scsi_set_sense`
- 外部函数：`scsi_sg_count`
- 外部函数：`scsi_sglist`
- 外部函数：`set_status_byte`
- 外部函数：`sg_copy_from_buffer`
- 外部函数：`sg_copy_to_buffer`
- 外部函数：`spin_lock_irqsave`
- 外部函数：`unlikely`
- 外部函数：`void`
- 外部函数：`xlat_func`
- 大写宏：`ABORTED_COMMAND`
- 大写宏：`ABRT`
- 大写宏：`ADDRESS_MARK_NOT_FOUND_FOR_DATA_FIELD`
- 大写宏：`ALLOW_MEDIUM_REMOVAL`
- 大写宏：`ATA`
- 大写宏：`ATA_BUSY`
- 大写宏：`ATA_CMD_SET_FEATURES`
- 大写宏：`ATA_CORR`
- 大写宏：`ATA_DF`
- 大写宏：`ATA_DFLAG_CDL`
- 大写宏：`ATA_DFLAG_CDL_ENABLED`
- 大写宏：`ATA_DFLAG_D_SENSE`
- 大写宏：`ATA_DFLAG_NCQ_PRIO_ENABLED`
- 大写宏：`ATA_DRQ`
- 大写宏：`ATA_ERR`
- 大写宏：`ATA_PASS_THROUGH_INFORMATION_AVAILABLE`
- 大写宏：`ATA_PFLAG_EH_PENDING`
- 大写宏：`ATA_PROT_NODATA`
- 大写宏：`ATA_QCFLAG_RTF_FILLED`
- 大写宏：`ATA_QCFLAG_SENSE_VALID`
- 大写宏：`ATA_SCSI_RBUF_SIZE`
- 大写宏：`ATA_TFLAG_DEVICE`
- 大写宏：`ATA_TFLAG_ISADDR`
- 大写宏：`ATA_TFLAG_WRITE`
- 大写宏：`BBD`
- 大写宏：`BUSY`
- 大写宏：`CDL`
- 大写宏：`CHECK`
- 大写宏：`CODE`
- 大写宏：`CONDITION`
- 大写宏：`CRC`
- 大写宏：`DID_OK`
- 大写宏：`DID_REQUEUE`
- 大写宏：`DID_TIME_OUT`
- 大写宏：`DMA_TO_DEVICE`
- 大写宏：`D_SENSE`
- 大写宏：`ECC`
- 大写宏：`EINVAL`
- 大写宏：`END`
- 大写宏：`FEATURES`
- 大写宏：`HARDWARE_ERROR`
- 大写宏：`ICRC`
- 大写宏：`IDNF`
- 大写宏：`ILLEGAL_REQUEST`
- 大写宏：`INQUIRY`
- 大写宏：`INTERNAL_TARGET_FAILURE`
- 大写宏：`LOCKING`
- 大写宏：`LOGICAL_BLOCK_ADDRESS_OUT_OF_RANGE`
- 大写宏：`LU_NOT_READY`
- 大写宏：`MAR`
- 大写宏：`MARK`
- 大写宏：`MCR`
- 大写宏：`MEDIUM_ERROR`
- 大写宏：`MODE`
- 大写宏：`NCQ`
- 大写宏：`NOT_READY`
- 大写宏：`NOT_READY_TO_READY_CHANGE_MEDIUM_MAY_HAVE_CHANGED`
- 大写宏：`NO_ADDITIONAL_SENSE_INFORMATION`
- 大写宏：`NULL`
- 大写宏：`PMP_FIRST`
- 大写宏：`RECOVERED_ERROR`
- 大写宏：`RESPONSE`
- 大写宏：`SAM_STAT_CHECK_CONDITION`
- 大写宏：`SAM_STAT_GOOD`
- 大写宏：`SCSI`
- 大写宏：`SCSI_EH_DONE`
- 大写宏：`SCSI_EH_NOT_HANDLED`
- 大写宏：`SCSI_PARITY_ERROR`
- 大写宏：`SELECT`
- 大写宏：`SET`
- 大写宏：`SETFEATURES_CDL`
- 大写宏：`SETFEATURES_WC_OFF`
- 大写宏：`SETFEATURES_WC_ON`
- 大写宏：`SRV`
- 大写宏：`TRK0`
- 大写宏：`UNIT_ATTENTION`
- 大写宏：`UNRECOVERED_READ_ERROR_AUTO_REALLOCATE_FAILED`
- 外部类型：`Abort`
- 外部类型：`All`
- 外部类型：`Bad`
- 外部类型：`Based`
- 外部类型：`Busy`
- 外部类型：`Changed`
- 外部类型：`Check`
- 外部类型：`Clear`
- 外部类型：`Control`
- 外部类型：`Corrected`
- 外部类型：`Data`
- 外部类型：`Device`
- 外部类型：`Disable`
- 外部类型：`Disabling`
- 外部类型：`Enable`
- 外部类型：`Enabling`
- 外部类型：`Fall`
- 外部类型：`Feature`
- 外部类型：`However`
- 外部类型：`If`
- 外部类型：`Ignore`
- 外部类型：`In`
- 外部类型：`Is`
- 外部类型：`Look`
- 外部类型：`Make`
- 外部类型：`Media`
- 外部类型：`None`
- 外部类型：`Note`
- 外部类型：`Only`
- 外部类型：`Prepare`
- 外部类型：`QCs`
- 外部类型：`Report`
- 外部类型：`Same`
- 外部类型：`Simulate`
- 外部类型：`Since`
- 外部类型：`So`
- 外部类型：`Sometimes`
- 外部类型：`Storage`
- 外部类型：`Supported`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`Track`
- 外部类型：`Translate`
- 外部类型：`Uncorrectable`
- 外部类型：`Unit`
- 外部类型：`Unrecovered`
- 外部类型：`Use`
- 外部类型：`We`
- 外部类型：`ata_device`
- 外部类型：`ata_link`
- 外部类型：`ata_port`
- 外部类型：`ata_queued_cmd`
- 外部类型：`ata_taskfile`
- 外部类型：`scsi_cmnd`
- 外部类型：`scsi_timeout_action`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-9add13e4a2` → 本草稿移入 `cases/defect/auto-linux-9add13e4a2/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
