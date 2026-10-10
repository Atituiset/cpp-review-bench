# auto-linux-b1c4a14aeb

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#b71ccf604fbdf78450d3b17b5d69365caba29061](https://github.com/torvalds/linux/commit/b71ccf604fbdf78450d3b17b5d69365caba29061) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-10 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 167 |
| 编译错误数（gcc syntax-only） | 172（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #b71ccf604fbdf78450d3b17b5d69365caba29061 (https://github.com/torvalds/linux/commit/b71ccf604fbdf78450d3b17b5d69365caba29061)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 830；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT b71ccf604fbdf78450d3b17b5d69365caba29061 Merge tag 'mmc-v7.3-rc1-2' of git://git.kernel.org/pub/scm/linux/kernel/git/ulfh/mmc :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -27,7 +27,6 @@ struct rtsx_usb_ms {
 	struct memstick_host	*msh;
 	struct memstick_request	*req;
 
-	struct mutex		host_mutex;
 	struct work_struct	handle_req;
 	struct delayed_work	poll_card;
 
@@ -514,6 +513,13 @@ static void rtsx_usb_ms_handle_req(struct work_struct *work)
 	struct memstick_host *msh = host->msh;
 	int rc;
 
+	/* Fail requests after eject so their waiters are released. */
+	if (host->eject) {
+		while (!memstick_next_req(msh, &host->req))
+			host->req->error = -ENOMEDIUM;
+		return;
+	}
+
 	if (!host->req) {
 		pm_runtime_get_sync(ms_dev(host));
 		do {
@@ -547,8 +553,7 @@ static void rtsx_usb_ms_request(struct memstick_host *msh)
 
 	dev_dbg(ms_dev(host), "--> %s\n", __func__);
 
-	if (!host->eject)
-		schedule_work(&host->handle_req);
+	schedule_work(&host->handle_req);
 }
 
 static int rtsx_usb_ms_set_param(struct memstick_host *msh,
@@ -781,7 +786,6 @@ static int rtsx_usb_ms_drv_probe(struct platform_device *pdev)
 	host->power_mode = MEMSTICK_POWER_OFF;
 	platform_set_drvdata(pdev, host);
 
-	mutex_init(&host->host_mutex);
 	INIT_WORK(&host->handle_req, rtsx_usb_ms_handle_req);
 
 	INIT_DELAYED_WORK(&host->poll_card, rtsx_usb_ms_poll_card);
@@ -812,27 +816,12 @@ static void rtsx_usb_ms_drv_remove(struct platform_device *pdev)
 {
 	struct rtsx_usb_ms *host = platform_get_drvdata(pdev);
 	struct memstick_host *msh = host->msh;
-	int err;
 
 	host->eject = true;
 	msh->removing = true;
-	cancel_work_sync(&host->handle_req);
+	flush_work(&host->handle_req);
 	cancel_delayed_work_sync(&host->poll_card);
 
-	mutex_lock(&host->host_mutex);
-	if (host->req) {
-		dev_dbg(ms_dev(host),
-			"%s: Controller removed during transfer\n",
-			dev_name(&msh->dev));
-		host->req->error = -ENOMEDIUM;
-		do {
-			err = memstick_next_req(msh, &host->req);
-			if (!err)
-				host->req->error = -ENOMEDIUM;
-		} while (!err);
-	}
-	mutex_unlock(&host->host_mutex);
-
 	/* Balance possible unbalanced usage count
 	 * e.g. unconditional module removal
 	 */
@@ -841,6 +830,8 @@ static void rtsx_usb_ms_drv_remove(struct platform_device *pdev)
 
 	pm_runtime_disable(ms_dev(host));
 	memstick_remove_host(msh);
+	/* No card, no new requests; wait for the last failed one to finish. */
+	cancel_work_sync(&host->handle_req);
 	dev_dbg(ms_dev(host),
 		": Realtek USB Memstick controller has been removed\n");
 	memstick_free_host(msh);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`cancel_delayed_work_sync`
- 外部函数：`cancel_work_sync`
- 外部函数：`container_of`
- 外部函数：`dev_dbg`
- 外部函数：`dev_name`
- 外部函数：`memstick_detect_change`
- 外部函数：`memstick_free_host`
- 外部函数：`memstick_next_req`
- 外部函数：`memstick_priv`
- 外部函数：`memstick_remove_host`
- 外部函数：`mutex_init`
- 外部函数：`mutex_lock`
- 外部函数：`mutex_unlock`
- 外部函数：`platform_get_drvdata`
- 外部函数：`platform_set_drvdata`
- 外部函数：`pm_runtime_disable`
- 外部函数：`pm_runtime_get_noresume`
- 外部函数：`pm_runtime_get_sync`
- 外部函数：`pm_runtime_put_noidle`
- 外部函数：`pm_runtime_put_sync`
- 外部函数：`rtsx_usb_add_cmd`
- 外部函数：`rtsx_usb_card_exclusive_check`
- 外部函数：`rtsx_usb_clear_dma_err`
- 外部函数：`rtsx_usb_clear_fsm_err`
- 外部函数：`rtsx_usb_ep0_read_register`
- 外部函数：`rtsx_usb_ep0_write_register`
- 外部函数：`rtsx_usb_get_rsp`
- 外部函数：`rtsx_usb_init_cmd`
- 外部函数：`rtsx_usb_read_register`
- 外部函数：`rtsx_usb_send_cmd`
- 外部函数：`rtsx_usb_switch_clock`
- 外部函数：`rtsx_usb_transfer_data`
- 外部函数：`rtsx_usb_write_register`
- 外部函数：`schedule_delayed_work`
- 外部函数：`schedule_work`
- 外部函数：`usb_rcvbulkpipe`
- 外部函数：`usb_sndbulkpipe`
- 外部函数：`usleep_range`
- 大写宏：`CARD_CLK_EN`
- 大写宏：`CARD_DATA_SOURCE`
- 大写宏：`CARD_GPIO`
- 大写宏：`CARD_INT_PEND`
- 大写宏：`CARD_OE`
- 大写宏：`CARD_PULL_CTL1`
- 大写宏：`CARD_PULL_CTL2`
- 大写宏：`CARD_PULL_CTL3`
- 大写宏：`CARD_PULL_CTL4`
- 大写宏：`CARD_PULL_CTL5`
- 大写宏：`CARD_PULL_CTL6`
- 大写宏：`CARD_PWR_CTL`
- 大写宏：`CARD_SELECT`
- 大写宏：`CARD_SHARE_MASK`
- 大写宏：`CARD_SHARE_MODE`
- 大写宏：`CARD_SHARE_MS`
- 大写宏：`CARD_STOP`
- 大写宏：`CHECK_PKG`
- 大写宏：`CHECK_REG_CMD`
- 大写宏：`DMA_512`
- 大写宏：`DMA_DIR_FROM_CARD`
- 大写宏：`DMA_DIR_TO_CARD`
- 大写宏：`DMA_EN`
- 大写宏：`DMA_PACK_SIZE_MASK`
- 大写宏：`EINVAL`
- 大写宏：`EIO`
- 大写宏：`ENOMEDIUM`
- 大写宏：`EP_BULK_IN`
- 大写宏：`EP_BULK_OUT`
- 大写宏：`ETIMEDOUT`
- 大写宏：`INIT_DELAYED_WORK`
- 大写宏：`INIT_WORK`
- 大写宏：`LDO3318_PWR_MASK`
- 大写宏：`LDO_SUSPEND`
- 大写宏：`LQFP48`
- 大写宏：`MC_DMA_CTL`
- 大写宏：`MC_DMA_TC0`
- 大写宏：`MC_DMA_TC1`
- 大写宏：`MC_DMA_TC2`
- 大写宏：`MC_DMA_TC3`
- 大写宏：`MEMSTICK_INTERFACE`
- 大写宏：`MEMSTICK_INT_BREQ`
- 大写宏：`MEMSTICK_INT_CED`
- 大写宏：`MEMSTICK_INT_CMDNAK`
- 大写宏：`MEMSTICK_INT_ERR`
- 大写宏：`MEMSTICK_PAR4`
- 大写宏：`MEMSTICK_POWER`
- 大写宏：`MEMSTICK_POWER_OFF`
- 大写宏：`MEMSTICK_POWER_ON`
- 大写宏：`MEMSTICK_SERIAL`
- 大写宏：`MEMSTICK_TYPE_PRO`
- 大写宏：`MODE_C`
- 大写宏：`MODE_CDIR`
- 大写宏：`MODE_CDOR`
- 大写宏：`MODE_CR`
- 大写宏：`MS_BUS_WIDTH_1`
- 大写宏：`MS_BUS_WIDTH_4`
- 大写宏：`MS_BYTE_CNT`
- 大写宏：`MS_CFG`
- 大写宏：`MS_CLK_EN`
- 大写宏：`MS_CLR_ERR`
- 大写宏：`MS_CRC16_ERR`
- 大写宏：`MS_INT`
- 大写宏：`MS_INT_BREQ`
- 大写宏：`MS_INT_CED`
- 大写宏：`MS_INT_CMDNK`
- 大写宏：`MS_INT_ERR`
- 大写宏：`MS_INT_REG`
- 大写宏：`MS_MOD_SEL`
- 大写宏：`MS_NO_CHECK_INT`
- 大写宏：`MS_OUTPUT_EN`
- 大写宏：`MS_RDY_TIMEOUT`
- 大写宏：`MS_SECTOR_CNT_H`
- 大写宏：`MS_SECTOR_CNT_L`
- 大写宏：`MS_STOP`
- 大写宏：`MS_TM_AUTO_READ`
- 大写宏：`MS_TM_AUTO_WRITE`
- 大写宏：`MS_TM_NORMAL_READ`
- 大写宏：`MS_TM_NORMAL_WRITE`
- 大写宏：`MS_TM_READ_BYTES`
- 大写宏：`MS_TM_WRITE_BYTES`
- 大写宏：`MS_TPC`
- 大写宏：`MS_TPC_GET_INT`
- 大写宏：`MS_TRANSFER`
- 大写宏：`MS_TRANSFER_END`
- 大写宏：`MS_TRANSFER_ERR`
- 大写宏：`MS_TRANSFER_START`
- 大写宏：`MS_TRANS_CFG`
- 大写宏：`NO_WAIT_INT`
- 大写宏：`NULL`
- 大写宏：`PARTIAL_POWER_ON`
- 大写宏：`PINGPONG_BUFFER`
- 大写宏：`POWER_MASK`
- 大写宏：`POWER_OFF`
- 大写宏：`POWER_ON`
- 大写宏：`PPBUF_BASE2`
- 大写宏：`PUSH_TIME_DEFAULT`
- 大写宏：`PUSH_TIME_ODD`
- 大写宏：`READ`
- 大写宏：`READ_REG_CMD`
- 大写宏：`RING_BUFFER`
- 大写宏：`RTSX_USB_MS_CARD`
- 大写宏：`SD_INT`
- 大写宏：`SSC_DEPTH_1M`
- 大写宏：`SSC_DEPTH_512K`
- 大写宏：`STAGE_MS_STATUS`
- 大写宏：`USB`
- 大写宏：`WAIT_INT`
- 大写宏：`WRITE`
- 大写宏：`WRITE_REG_CMD`
- 大写宏：`XD_INT`
- 外部类型：`Balance`
- 外部类型：`CARD_PULL_CTLx`
- 外部类型：`Check`
- 外部类型：`Clear`
- 外部类型：`Controller`
- 外部类型：`Memstick`
- 外部类型：`Print`
- 外部类型：`Realtek`
- 外部类型：`delayed_work`
- 外部类型：`device`
- 外部类型：`memstick_dev`
- 外部类型：`memstick_host`
- 外部类型：`memstick_param`
- 外部类型：`memstick_request`
- 外部类型：`mutex`
- 外部类型：`platform_device`
- 外部类型：`rtsx_ucr`
- 外部类型：`scatterlist`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-b1c4a14aeb` → 本草稿移入 `cases/defect/auto-linux-b1c4a14aeb/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
