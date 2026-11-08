# auto-linux-f699e39319

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#a41e364995bbcfa627418a70fe913c35b7510728](https://github.com/torvalds/linux/commit/a41e364995bbcfa627418a70fe913c35b7510728) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-08 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 89 |
| 编译错误数（gcc syntax-only） | 5（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #a41e364995bbcfa627418a70fe913c35b7510728 (https://github.com/torvalds/linux/commit/a41e364995bbcfa627418a70fe913c35b7510728)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 279；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT a41e364995bbcfa627418a70fe913c35b7510728 Merge tag 'scsi-fixes' of git://git.kernel.org/pub/scm/linux/kernel/git/jejb/scsi :: PR 修复动作推断：修复前解引用（疑似缺判空）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -276,8 +276,10 @@ static void blogic_create_addlccbs(struct blogic_adapter *adapter,
   Lock should already have been acquired by the caller.
 */
 
-static struct blogic_ccb *blogic_alloc_ccb(struct blogic_adapter *adapter)
+static struct blogic_ccb *blogic_alloc_ccb(struct Scsi_Host *host)
+	__must_hold(&host->host_lock)
 {
+	struct blogic_adapter *adapter = (void *)host->hostdata;
 	static unsigned long serial;
 	struct blogic_ccb *ccb;
 	ccb = adapter->free_ccbs;
@@ -307,7 +309,9 @@ static struct blogic_ccb *blogic_alloc_ccb(struct blogic_adapter *adapter)
   caller.
 */
 
-static void blogic_dealloc_ccb(struct blogic_ccb *ccb, int dma_unmap)
+static void blogic_dealloc_ccb(struct Scsi_Host *host, struct blogic_ccb *ccb,
+			       int dma_unmap)
+	__must_hold(&host->host_lock)
 {
 	struct blogic_adapter *adapter = ccb->adapter;
 
@@ -2596,8 +2600,11 @@ static void blogic_scan_inbox(struct blogic_adapter *adapter)
   should already have been acquired by the caller.
 */
 
-static void blogic_process_ccbs(struct blogic_adapter *adapter)
+static void blogic_process_ccbs(struct Scsi_Host *host)
+	__must_hold(&host->host_lock)
 {
+	struct blogic_adapter *adapter = (void *)host->hostdata;
+
 	if (adapter->processing_ccbs)
 		return;
 	adapter->processing_ccbs = true;
@@ -2621,7 +2628,7 @@ static void blogic_process_ccbs(struct blogic_adapter *adapter)
 			/*
 			   Place CCB back on the Host Adapter's free list.
 			 */
-			blogic_dealloc_ccb(ccb, 1);
+			blogic_dealloc_ccb(host, ccb, 1);
 #if 0			/* this needs to be redone different for new EH */
 			/*
 			   Bus Device Reset CCBs have the command field
@@ -2650,7 +2657,7 @@ static void blogic_process_ccbs(struct blogic_adapter *adapter)
 				if (ccb->status == BLOGIC_CCB_RESET &&
 						ccb->tgt_id == tgt_id) {
 					command = ccb->command;
-					blogic_dealloc_ccb(ccb, 1);
+					blogic_dealloc_ccb(host, ccb, 1);
 					adapter->active_cmds[tgt_id]--;
 					command->result = DID_RESET << 16;
 					scsi_done(command);
@@ -2721,7 +2728,7 @@ static void blogic_process_ccbs(struct blogic_adapter *adapter)
 			/*
 			   Place CCB back on the Host Adapter's free list.
 			 */
-			blogic_dealloc_ccb(ccb, 1);
+			blogic_dealloc_ccb(host, ccb, 1);
 			/*
 			   Call the SCSI Command Completion Routine.
 			 */
@@ -2740,11 +2747,12 @@ static void blogic_process_ccbs(struct blogic_adapter *adapter)
 static irqreturn_t blogic_inthandler(int irq_ch, void *devid)
 {
 	struct blogic_adapter *adapter = (struct blogic_adapter *) devid;
+	struct Scsi_Host *host = adapter->scsi_host;
 	unsigned long processor_flag;
 	/*
 	   Acquire exclusive access to Host Adapter.
 	 */
-	spin_lock_irqsave(&adapter->scsi_host->host_lock, processor_flag);
+	spin_lock_irqsave(&host->host_lock, processor_flag);
 	/*
 	   Handle Interrupts appropriately for each Host Adapter type.
 	 */
@@ -2794,25 +2802,25 @@ static irqreturn_t blogic_inthandler(int irq_ch, void *devid)
 	   Process any completed CCBs.
 	 */
 	if (adapter->firstccb != NULL)
-		blogic_process_ccbs(adapter);
+		blogic_process_ccbs(host);
 	/*
 	   Reset the Host Adapter if requested.
 	 */
 	if (adapter->adapter_extreset) {
 		blogic_warn("Resetting %s due to External SCSI Bus Reset\n", adapter, adapter->full_model);
 		blogic_inc_count(&adapter->ext_resets);
-		blogic_resetadapter(adapter, false);
+		blogic_resetadapter(host, false);
 		adapter->adapter_extreset = false;
 	} else if (adapter->adapter_intern_err) {
 		blogic_warn("Resetting %s due to Host Adapter Internal Error\n", adapter, adapter->full_model);
 		blogic_inc_count(&adapter->adapter_intern_errors);
-		blogic_resetadapter(adapter, true);
+		blogic_resetadapter(host, true);
 		adapter->adapter_intern_err = false;
 	}
 	/*
 	   Release exclusive access to Host Adapter.
 	 */
-	spin_unlock_irqrestore(&adapter->scsi_host->host_lock, processor_flag);
+	spin_unlock_irqrestore(&host->host_lock, processor_flag);
 	return IRQ_HANDLED;
 }
 
@@ -2823,9 +2831,11 @@ static irqreturn_t blogic_i
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`blogic_delay`
- 外部函数：`blogic_err`
- 外部函数：`blogic_inc_count`
- 外部函数：`blogic_resetadapter`
- 外部函数：`blogic_warn`
- 外部函数：`comp_cb`
- 外部函数：`dma_mapping_error`
- 外部函数：`scsi_done`
- 外部函数：`spin_lock_irq`
- 外部函数：`spin_lock_irqsave`
- 外部函数：`spin_unlock_irq`
- 外部函数：`spin_unlock_irqrestore`
- 外部函数：`void`
- 大写宏：`BLOGIC_CCB_COMPLETE`
- 大写宏：`BLOGIC_CCB_RESET`
- 大写宏：`BLOGIC_MBOX_START`
- 大写宏：`CCB`
- 大写宏：`DEF_SCSI_QCMD`
- 大写宏：`DID_ERROR`
- 大写宏：`DID_RESET`
- 大写宏：`DMA`
- 大写宏：`IRQ_HANDLED`
- 大写宏：`NULL`
- 大写宏：`SCSI`
- 大写宏：`SCSI_MLQUEUE_HOST_BUSY`
- 大写宏：`SUCCESS`
- 外部类型：`Abort`
- 外部类型：`Acquire`
- 外部类型：`Active`
- 外部类型：`Adapter`
- 外部类型：`Attempt`
- 外部类型：`Bus`
- 外部类型：`CCBs`
- 外部类型：`Call`
- 外部类型：`Command`
- 外部类型：`Completion`
- 外部类型：`Dead`
- 外部类型：`Device`
- 外部类型：`Error`
- 外部类型：`External`
- 外部类型：`Found`
- 外部类型：`Handle`
- 外部类型：`Hard`
- 外部类型：`Host`
- 外部类型：`If`
- 外部类型：`Internal`
- 外部类型：`Interrupts`
- 外部类型：`Lock`
- 外部类型：`Mailbox`
- 外部类型：`No`
- 外部类型：`Outgoing`
- 外部类型：`Pausing`
- 外部类型：`Place`
- 外部类型：`Process`
- 外部类型：`Release`
- 外部类型：`Reset`
- 外部类型：`Resetting`
- 外部类型：`Routine`
- 外部类型：`SCpnt`
- 外部类型：`Still`
- 外部类型：`Target`
- 外部类型：`Unable`
- 外部类型：`blogic_action`
- 外部类型：`blogic_adapter`
- 外部类型：`blogic_ccb`
- 外部类型：`blogic_outbox`
- 外部类型：`blogic_tgt_flags`
- 外部类型：`blogic_tgt_stats`
- 外部类型：`irqreturn_t`
- 外部类型：`scsi_cmnd`
- 外部类型：`scsi_qc_status`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-f699e39319` → 本草稿移入 `cases/defect/auto-linux-f699e39319/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
