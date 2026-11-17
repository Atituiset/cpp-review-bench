# auto-linux-8d303f837b

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#f11fe7603df07fbb9fa02b0d385c1696c87a6dab](https://github.com/torvalds/linux/commit/f11fe7603df07fbb9fa02b0d385c1696c87a6dab) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-17 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 291 |
| 编译错误数（gcc syntax-only） | 506（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #f11fe7603df07fbb9fa02b0d385c1696c87a6dab (https://github.com/torvalds/linux/commit/f11fe7603df07fbb9fa02b0d385c1696c87a6dab)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 1799；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT f11fe7603df07fbb9fa02b0d385c1696c87a6dab Merge tag 'usb-7.3-rc4' of git://git.kernel.org/pub/scm/linux/kernel/git/gregkh/usb :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -906,6 +906,54 @@ static int dwc3_gadget_resize_tx_fifos(struct dwc3_ep *dep)
 	return 0;
 }
 
+static int dwc3_gadget_ep_start_noop_transfer(struct dwc3_ep *dep)
+{
+	struct dwc3_gadget_ep_cmd_params params;
+	struct dwc3		*dwc = dep->dwc;
+	struct dwc3_trb		*trb;
+	dma_addr_t		trb_dma;
+	u32			cmd;
+	int			ret;
+
+	memset(&params, 0, sizeof(params));
+	trb = &dep->trb_pool[0];
+	trb_dma = dwc3_trb_dma_offset(dep, trb);
+
+	params.param0 = upper_32_bits(trb_dma);
+	params.param1 = lower_32_bits(trb_dma);
+
+	cmd = DWC3_DEPCMD_STARTTRANSFER;
+
+	ret = dwc3_send_gadget_ep_cmd(dep, cmd, &params);
+	if (ret < 0)
+		return ret;
+
+	/*
+	 * For streams, at start, there maybe a race where the host primes the
+	 * endpoint before the function driver queues a request to initiate a
+	 * stream. In that case, the controller will not see the prime to
+	 * generate the ERDY and start stream. To workaround this, issue a
+	 * no-op TRB as normal, but end it immediately. As a result, when the
+	 * function driver queues the request, the next START_TRANSFER command
+	 * will cause the controller to generate an ERDY to initiate the
+	 * stream.
+	 */
+	dwc3_stop_active_transfer(dep, false, true);
+
+	/*
+	 * All stream eps will reinitiate stream on NoStream rejection.
+	 *
+	 * However, if the controller is capable of TXF_FLUSH_BYPASS, then IN
+	 * direction endpoints will automatically restart the stream without
+	 * the driver initiation.
+	 */
+	if (!dep->direction ||
+	    !(dwc->hwparams.hwparams9 & DWC3_GHWPARAMS9_DEV_TXF_FLUSH_BYPASS))
+		dep->flags |= DWC3_EP_FORCE_RESTART_STREAM;
+
+	return 0;
+}
+
 /**
  * __dwc3_gadget_ep_enable - initializes a hw endpoint
  * @dep: endpoint to be initialized
@@ -971,54 +1019,14 @@ static int __dwc3_gadget_ep_enable(struct dwc3_ep *dep, unsigned int action)
 	 * Issue StartTransfer here with no-op TRB so we can always rely on No
 	 * Response Update Transfer command.
 	 */
-	if (usb_endpoint_xfer_bulk(desc) ||
-			usb_endpoint_xfer_int(desc)) {
-		struct dwc3_gadget_ep_cmd_params params;
-		struct dwc3_trb	*trb;
-		dma_addr_t trb_dma;
-		u32 cmd;
-
-		memset(&params, 0, sizeof(params));
-		trb = &dep->trb_pool[0];
-		trb_dma = dwc3_trb_dma_offset(dep, trb);
-
-		params.param0 = upper_32_bits(trb_dma);
-		params.param1 = lower_32_bits(trb_dma);
-
-		cmd = DWC3_DEPCMD_STARTTRANSFER;
-
-		ret = dwc3_send_gadget_ep_cmd(dep, cmd, &params);
-		if (ret < 0)
-			return ret;
-
-		if (dep->stream_capable) {
-			/*
-			 * For streams, at start, there maybe a race where the
-			 * host primes the endpoint before the function driver
-			 * queues a request to initiate a stream. In that case,
-			 * the controller will not see the prime to generate the
-			 * ERDY and start stream. To workaround this, issue a
-			 * no-op TRB as normal, but end it immediately. As a
-			 * result, when the function driver queues the request,
-			 * the next START_TRANSFER command will cause the
-			 * controller to generate an ERDY to initiate the
-			 * stream.
-			 */
-			dwc3_stop_active_transfer(dep, false, true);
-
-			/*
-			 * All stream eps will reinitiate stream on NoStream
-			 * rejection.
-			 *
-			 * However, if the controller is capable of
-			 * TXF_FLUSH_BYPASS, then IN direction endpoints will
-			 * automatically restart the stream without the driver
-			 * initiation.
-			 */
-			if (!dep->direction ||
-			    !(dwc->hwparams.hwparams9 &
-			      DWC3_GHWPARAMS9_DEV_TXF_FLUSH_BYPASS))
-				dep->flags |= DWC3_EP_FORCE_RESTART_STREAM;
+	if (dep->stream_capable) {
+		if (dep->flags & (DWC3_EP_END_TRANSFER_PENDING |
+				  DWC3_EP_DELAY_STOP)) {
+			dep->flags |= DWC3_EP_PENDING_NOOP_START;
+		} else {
+			ret = dwc3_gadget_ep_start_noop_transfer(dep);
+			if (ret < 0)
+				return ret;
 		}
 	}
 
@@ -1028,16 +1036,21 @@ static int __dwc3_gadget_ep_enable(struct dwc3_ep *dep, unsigned int action)
 	return 0;
 }
 
-void dwc3_remove_requests(struct dwc3 *dwc, struct dwc3_ep *dep, int status)
+void dwc3_remove_requ
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`dev_WARN`
- 外部函数：`dev_dbg`
- 外部函数：`dev_err`
- 外部函数：`dev_err_ratelimited`
- 外部函数：`dwc3_ep0_send_delayed_status`
- 外部函数：`dwc3_gadget_ep_get_transfer_index`
- 外部函数：`dwc3_gadget_move_cancelled_request`
- 外部函数：`dwc3_gadget_move_started_request`
- 外部函数：`dwc3_mdwidth`
- 外部函数：`dwc3_readl`
- 外部函数：`dwc3_writel`
- 外部函数：`list_del`
- 外部函数：`list_empty`
- 外部函数：`lower_32_bits`
- 外部函数：`max_t`
- 外部函数：`mdelay`
- 外部函数：`memset`
- 外部函数：`min_t`
- 外部函数：`next_request`
- 外部函数：`pm_runtime_put`
- 外部函数：`prepared`
- 外部函数：`sg_dma_address`
- 外部函数：`sg_dma_len`
- 外部函数：`sg_next`
- 外部函数：`size`
- 外部函数：`spin_lock`
- 外部函数：`spin_unlock`
- 外部函数：`trace_dwc3_gadget_ep_cmd`
- 外部函数：`trace_dwc3_gadget_ep_enable`
- 外部函数：`trace_dwc3_gadget_giveback`
- 外部函数：`trace_dwc3_prepare_trb`
- 外部函数：`unlikely`
- 外部函数：`upper_32_bits`
- 外部函数：`usb_endpoint_dir_in`
- 外部函数：`usb_endpoint_dir_out`
- 外部函数：`usb_endpoint_maxp`
- 外部函数：`usb_endpoint_maxp_mult`
- 外部函数：`usb_endpoint_type`
- 外部函数：`usb_endpoint_xfer_bulk`
- 外部函数：`usb_endpoint_xfer_control`
- 外部函数：`usb_endpoint_xfer_int`
- 外部函数：`usb_endpoint_xfer_isoc`
- 外部函数：`usb_gadget_giveback_request`
- 外部函数：`usb_gadget_map_request_by_dev`
- 外部函数：`usb_gadget_unmap_request_by_dev`
- 外部函数：`usb_ss_max_streams`
- 外部函数：`wmb`
- 大写宏：`BUILD_BUG_ON_NOT_POWER_OF_2`
- 大写宏：`CMDACT`
- 大写宏：`CMDIOC`
- 大写宏：`CPU`
- 大写宏：`CSP`
- 大写宏：`DATA0`
- 大写宏：`DATA1`
- 大写宏：`DATA2`
- 大写宏：`DEPCFG`
- 大写宏：`DEPCMD`
- 大写宏：`DEPEVT_TRANSFER_BUS_EXPIRY`
- 大写宏：`DEPEVT_TRANSFER_NO_RESOURCE`
- 大写宏：`DMA`
- 大写宏：`DWC3`
- 大写宏：`DWC31_GTXFIFOSIZ_TXFDEP`
- 大写宏：`DWC3_DALEPENA`
- 大写宏：`DWC3_DALEPENA_EP`
- 大写宏：`DWC3_DEPCFG_ACTION_INIT`
- 大写宏：`DWC3_DEPCFG_ACTION_RESTORE`
- 大写宏：`DWC3_DEPCFG_BINTERVAL_M1`
- 大写宏：`DWC3_DEPCFG_BURST_SIZE`
- 大写宏：`DWC3_DEPCFG_EP_NUMBER`
- 大写宏：`DWC3_DEPCFG_EP_TYPE`
- 大写宏：`DWC3_DEPCFG_FIFO_NUMBER`
- 大写宏：`DWC3_DEPCFG_MAX_PACKET_SIZE`
- 大写宏：`DWC3_DEPCFG_STREAM_CAPABLE`
- 大写宏：`DWC3_DEPCFG_STREAM_EVENT_EN`
- 大写宏：`DWC3_DEPCFG_XFER_COMPLETE_EN`
- 大写宏：`DWC3_DEPCFG_XFER_IN_PROGRESS_EN`
- 大写宏：`DWC3_DEPCFG_XFER_NOT_READY_EN`
- 大写宏：`DWC3_DEPCMD`
- 大写宏：`DWC3_DEPCMDPAR0`
- 大写宏：`DWC3_DEPCMDPAR1`
- 大写宏：`DWC3_DEPCMDPAR2`
- 大写宏：`DWC3_DEPCMD_CMD`
- 大写宏：`DWC3_DEPCMD_CMDACT`
- 大写宏：`DWC3_DEPCMD_CMDIOC`
- 大写宏：`DWC3_DEPCMD_ENDTRANSFER`
- 大写宏：`DWC3_DEPCMD_HIPRI_FORCERM`
- 大写宏：`DWC3_DEPCMD_PARAM`
- 大写宏：`DWC3_DEPCMD_SETEPCONFIG`
- 大写宏：`DWC3_DEPCMD_SETTRANSFRESOURCE`
- 大写宏：`DWC3_DEPCMD_STARTTRANSFER`
- 大写宏：`DWC3_DEPCMD_STATUS`
- 大写宏：`DWC3_DEPCMD_UPDATETRANSFER`
- 大写宏：`DWC3_DEPXFERCFG_NUM_XFER_RES`
- 大写宏：`DWC3_DSTS_SOFFN`
- 大写宏：`DWC3_EP_DELAY_START`
- 大写宏：`DWC3_EP_DELAY_STOP`
- 大写宏：`DWC3_EP_ENABLED`
- 大写宏：`DWC3_EP_END_TRANSFER_PENDING`
- 大写宏：`DWC3_EP_FORCE_RESTART_STREAM`
- 大写宏：`DWC3_EP_IGNORE_NEXT_NOSTREAM`
- 大写宏：`DWC3_EP_PENDING_REQUEST`
- 大写宏：`DWC3_EP_RESOURCE_ALLOCATED`
- 大写宏：`DWC3_EP_TRANSFER_STARTED`
- 大写宏：`DWC3_EP_TXFIFO_RESIZED`
- 大写宏：`DWC3_EP_WAIT_TRANSFER_COMPLETE`
- 大写宏：`DWC3_GHWPARAMS9_DEV_TXF_FLUSH_BYPASS`
- 大写宏：`DWC3_GTXFIFOSIZ`
- 大写宏：`DWC3_GTXFIFOSIZ_TXFDEP`
- 大写宏：`DWC3_GTXFIFOSIZ_TXFSTADDR`
- 大写宏：`DWC3_GUSB2PHYCFG`
- 大写宏：`DWC3_GUSB2PHYCFG_ENBLSLPM`
- 大写宏：`DWC3_GUSB2PHYCFG_SUSPHY`
- 大写宏：`DWC3_IP_IS`
- 大写宏：`DWC3_MST_CAPABLE`
- 大写宏：`DWC3_RAM0_DEPTH`
- 大写宏：`DWC3_RAM1_DEPTH`
- 大写宏：`DWC3_REQUEST_STATUS_COMPLETED`
- 大写宏：`DWC3_REQUEST_STATUS_DEQUEUED`
- 大写宏：`DWC3_REQUEST_STATUS_DISCONNECTED`
- 大写宏：`DWC3_REQUEST_STATUS_STALLED`
- 大写宏：`DWC3_SPRAM_TYPE`
- 大写宏：`DWC3_TRBCTL_CONTROL_SETUP`
- 大写宏：`DWC3_TRBCTL_ISOCHRONOUS`
- 大写宏：`DWC3_TRBCTL_ISOCHRONOUS_FIRST`
- 大写宏：`DWC3_TRBCTL_LINK_TRB`
- 大写宏：`DWC3_TRBCTL_NORMAL`
- 大写宏：`DWC3_TRB_CTRL_CHN`
- 大写宏：`DWC3_TRB_CTRL_CSP`
- 大写宏：`DWC3_TRB_CTRL_HWO`
- 大写宏：`DWC3_TRB_CTRL_IOC`
- 大写宏：`DWC3_TRB_CTRL_ISP_IMI`
- 大写宏：`DWC3_TRB_CTRL_LST`
- 大写宏：`DWC3_TRB_CTRL_SID_SOFN`
- 大写宏：`DWC3_TRB_NUM`
- 大写宏：`DWC3_TRB_SIZE_LENGTH`
- 大写宏：`DWC3_TRB_SIZE_PCM1`
- 大写宏：`DWC3_VER_IS_PRIOR`
- 大写宏：`DWORD`
- 大写宏：`EAGAIN`
- 大写宏：`ECONNRESET`
- 大写宏：`EINPROGRESS`
- 大写宏：`EINVAL`
- 大写宏：`ENBLSLPM`
- 大写宏：`END_TRANSFER`
- 大写宏：`ENOMEM`
- 大写宏：`EP0_DATA_PHASE`
- 大写宏：`EP0_SETUP_PHASE`
- 大写宏：`EPIPE`
- 大写宏：`ERDY`
- 大写宏：`ESHUTDOWN`
- 大写宏：`ETIMEDOUT`
- 大写宏：`FIFO`
- 大写宏：`GUCTL2`
- 大写宏：`GUSB2PHYCFG`
- 大写宏：`HWO`
- 大写宏：`IOC`
- 大写宏：`IOMMU`
- 大写宏：`IOW`
- 大写宏：`IRQ`
- 大写宏：`LST`
- 大写宏：`MDWIDTH`
- 大写宏：`MPS`
- 大写宏：`MST`
- 大写宏：`NOT`
- 大写宏：`NOTE`
- 大写宏：`NOTICE`
- 大写宏：`NULL`
- 大写宏：`OUT`
- 大写宏：`PID`
- 大写宏：`RAM`
- 大写宏：`RAM0`
- 大写宏：`RAM1`
- 大写宏：`SETUP`
- 大写宏：`START`
- 大写宏：`START_TRANSFER`
- 大写宏：`SUSPHY`
- 大写宏：`TRANSFER`
- 大写宏：`TRB`
- 大写宏：`TXF_FLUSH_BYPASS`
- 大写宏：`UNKNOWN`
- 大写宏：`USB`
- 大写宏：`USB_ENDPOINT_XFER_BULK`
- 大写宏：`USB_ENDPOINT_XFER_CONTROL`
- 大写宏：`USB_ENDPOINT_XFER_INT`
- 大写宏：`USB_ENDPOINT_XFER_ISOC`
- 大写宏：`USB_SPEED_FULL`
- 大写宏：`USB_SPEED_HIGH`
- 大写宏：`USB_SPEED_SUPER`
- 大写宏：`USB_SPEED_SUPER_PLUS`
- 大写宏：`ZLP`
- 外部类型：`Add`
- 外部类型：`All`
- 外部类型：`As`
- 外部类型：`Bit`
- 外部类型：`Burst`
- 外部类型：`Bus`
- 外部类型：`Calculate`
- 外部类型：`Check`
- 外部类型：`Clear`
- 外部类型：`CmdAct`
- 外部类型：`CmdIOC`
- 外部类型：`Command`
- 外部类型：`Completion`
- 外部类型：`Continue`
- 外部类型：`Control`
- 外部类型：`DEPCMDPARn`
- 外部类型：`DWC_usb3`
- 外部类型：`DWC_usb31`
- 外部类型：`DWC_usb32`
- 外部类型：`DWC_usb3x`
- 外部类型：`Data`
- 外部类型：`Databook`
- 外部类型：`Delay`
- 外部类型：`Don`
- 外部类型：`EPs`
- 外部类型：`Enable`
- 外部类型：`End`
- 外部类型：`EndTransfer`
- 外部类型：`Endpoint`
- 外部类型：`Endpoints`
- 外部类型：`Expiry`
- 外部类型：`FIFOs`
- 外部类型：`For`
- 外部类型：`However`
- 外部类型：`Ideally`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Initialize`
- 外部类型：`Instead`
- 外部类型：`Internally`
- 外部类型：`Issue`
- 外部类型：`It`
- 外部类型：`Last`
- 外部类型：`Let`
- 外部类型：`Link`
- 外部类型：`Logical`
- 外部类型：`Look`
- 外部类型：`No`
- 外部类型：`NoStream`
- 外部类型：`Note`
- 外部类型：`Only`
- 外部类型：`Packet`
- 外部类型：`Paul`
- 外部类型：`Phase`
- 外部类型：`Physical`
- 外部类型：`Polling`
- 外部类型：`Prepare`
- 外部类型：`Provider`
- 外部类型：`Reserve`
- 外部类型：`Response`
- 外部类型：`Restore`
- 外部类型：`Rules`
- 外部类型：`Save`
- 外部类型：`Section`
- 外部类型：`See`
- 外部类型：`Setup`
- 外部类型：`Short`
- 外部类型：`Since`
- 外部类型：`Skip`
- 外部类型：`So`
- 外部类型：`Specification`
- 外部类型：`StartTransfer`
- 外部类型：`SuperSpeed`
- 外部类型：`Synopsys`
- 外部类型：`TRBs`
- 外部类型：`TXFIFOs`
- 外部类型：`The`
- 外部类型：`There`
- 外部类型：`Therefore`
- 外部类型：`This`
- 外部类型：`To`
- 外部类型：`Transfer`
- 外部类型：`TxFIFOs`
- 外部类型：`Typically`
- 外部类型：`Unknown`
- 外部类型：`Update`
- 外部类型：`Valid`
- 外部类型：`We`
- 外部类型：`When`
- 外部类型：`With`
- 外部类型：`XferNotReady`
- 外部类型：`dma_addr_t`
- 外部类型：`dwc3`
- 外部类型：`dwc3_ep`
- 外部类型：`dwc3_gadget_ep_cmd_params`
- 外部类型：`dwc3_request`
- 外部类型：`dwc3_trb`
- 外部类型：`scatterlist`
- 外部类型：`usb_device_speed`
- 外部类型：`usb_endpoint_descriptor`
- 外部类型：`usb_ep`
- 外部类型：`usb_gadget`
- 外部类型：`usb_ss_ep_comp_descriptor`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-8d303f837b` → 本草稿移入 `cases/defect/auto-linux-8d303f837b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
