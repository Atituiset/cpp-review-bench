# auto-linux-ad265511de

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#b62f58bd7187ddbefa66913ec742137680ca5b22](https://github.com/torvalds/linux/commit/b62f58bd7187ddbefa66913ec742137680ca5b22) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-11 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 108 |
| 编译错误数（gcc syntax-only） | 111（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #b62f58bd7187ddbefa66913ec742137680ca5b22 (https://github.com/torvalds/linux/commit/b62f58bd7187ddbefa66913ec742137680ca5b22)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 1006；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT b62f58bd7187ddbefa66913ec742137680ca5b22 Merge tag 'i2c-fixes-7.3-rc7' of git://git.kernel.org/pub/scm/linux/kernel/git/andi.shyti/linux :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1003,13 +1003,14 @@ static int geni_i2c_xfer(struct i2c_adapter *adap,
 
 	ret = gi2c->dev_data->set_rate(&gi2c->se, gi2c->clk_freq_out);
 	if (ret)
-		return ret;
+		goto out_pm;
 
 	if (gi2c->gpi_mode)
 		ret = geni_i2c_gpi_xfer(gi2c, msgs, num);
 	else
 		ret = geni_i2c_fifo_xfer(gi2c, msgs, num);
 
+out_pm:
 	pm_runtime_put_autosuspend(gi2c->se.dev);
 	gi2c->cur = NULL;
 	gi2c->err = 0;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`complete`
- 外部函数：`dev_dbg`
- 外部函数：`dev_err`
- 外部函数：`dma_async_issue_pending`
- 外部函数：`dma_map_single`
- 外部函数：`dma_mapping_error`
- 外部函数：`dma_submit_error`
- 外部函数：`dma_unmap_single`
- 外部函数：`dmaengine_prep_slave_single`
- 外部函数：`dmaengine_slave_config`
- 外部函数：`dmaengine_submit`
- 外部函数：`dmaengine_terminate_sync`
- 外部函数：`failed`
- 外部函数：`geni_se_abort_m_cmd`
- 外部函数：`geni_se_cancel_m_cmd`
- 外部函数：`geni_se_rx_dma_prep`
- 外部函数：`geni_se_rx_dma_unprep`
- 外部函数：`geni_se_select_mode`
- 外部函数：`geni_se_setup_m_cmd`
- 外部函数：`geni_se_tx_dma_prep`
- 外部函数：`geni_se_tx_dma_unprep`
- 外部函数：`i2c_get_dma_safe_msg_buf`
- 外部函数：`i2c_put_dma_safe_msg_buf`
- 外部函数：`int`
- 外部函数：`kcalloc`
- 外部函数：`kfree`
- 外部函数：`kzalloc_objs`
- 外部函数：`memset`
- 外部函数：`msecs_to_jiffies`
- 外部函数：`pm_runtime_put_autosuspend`
- 外部函数：`readl_relaxed`
- 外部函数：`reinit_completion`
- 外部函数：`set_rate`
- 外部函数：`spin_lock_irqsave`
- 外部函数：`spin_unlock_irqrestore`
- 外部函数：`trace_geni_i2c_bus_setup`
- 外部函数：`trace_geni_i2c_err`
- 外部函数：`wait_for_completion_timeout`
- 外部函数：`writel_relaxed`
- 大写宏：`ADDR_NACK`
- 大写宏：`BEI`
- 大写宏：`BIT`
- 大写宏：`DATA_NACK`
- 大写宏：`DMA`
- 大写宏：`DMA_CTRL_ACK`
- 大写宏：`DMA_DEV_TO_MEM`
- 大写宏：`DMA_FROM_DEVICE`
- 大写宏：`DMA_MEM_TO_DEV`
- 大写宏：`DMA_PREP_INTERRUPT`
- 大写宏：`DMA_TO_DEVICE`
- 大写宏：`DMA_TRANS_NOERROR`
- 大写宏：`EINVAL`
- 大写宏：`EIO`
- 大写宏：`ENOMEM`
- 大写宏：`ETIMEDOUT`
- 大写宏：`FIFO`
- 大写宏：`GENI_ABORT_DONE`
- 大写宏：`GENI_SE_DMA`
- 大写宏：`GENI_SE_FIFO`
- 大写宏：`GENI_TIMEOUT`
- 大写宏：`GENMASK`
- 大写宏：`GFP_KERNEL`
- 大写宏：`GPI`
- 大写宏：`I2C`
- 大写宏：`I2C_M_RD`
- 大写宏：`IRQ`
- 大写宏：`NULL`
- 大写宏：`RX_FSM`
- 大写宏：`SE_DMA_RX_FSM_RST`
- 大写宏：`SE_DMA_RX_IRQ_STAT`
- 大写宏：`SE_DMA_TX_FSM_RST`
- 大写宏：`SE_DMA_TX_IRQ_STAT`
- 大写宏：`SE_GENI_DMA_MODE_EN`
- 大写宏：`SE_GENI_IOS`
- 大写宏：`SE_GENI_M_CMD0`
- 大写宏：`SE_GENI_M_IRQ_STATUS`
- 大写宏：`SE_GENI_RX_FIFO_STATUS`
- 大写宏：`SE_GENI_STATUS`
- 大写宏：`SE_GENI_TX_FIFO_STATUS`
- 大写宏：`SE_GENI_TX_WATERMARK_REG`
- 大写宏：`TODO`
- 大写宏：`TRE`
- 大写宏：`TX_FSM`
- 外部类型：`Add`
- 外部类型：`Avoid`
- 外部类型：`Get`
- 外部类型：`If`
- 外部类型：`Multi`
- 外部类型：`Non`
- 外部类型：`Retry`
- 外部类型：`Skip`
- 外部类型：`TREs`
- 外部类型：`Timeout`
- 外部类型：`Transfer`
- 外部类型：`Unmap`
- 外部类型：`completion`
- 外部类型：`device`
- 外部类型：`dma_addr_t`
- 外部类型：`dma_async_tx_descriptor`
- 外部类型：`dma_chan`
- 外部类型：`dma_cookie_t`
- 外部类型：`dma_data_direction`
- 外部类型：`dma_slave_config`
- 外部类型：`dma_transfer_direction`
- 外部类型：`dmaengine_result`
- 外部类型：`geni_i2c_gpi_multi_desc_xfer`
- 外部类型：`geni_se`
- 外部类型：`gpi_i2c_config`
- 外部类型：`i2c_adapter`
- 外部类型：`i2c_msg`
- 外部类型：`size_t`
- 外部类型：`spinlock_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-ad265511de` → 本草稿移入 `cases/defect/auto-linux-ad265511de/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
