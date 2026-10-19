# auto-linux-6af26b8934

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#8486d357129126e27fb5d4e02622566bafc77035](https://github.com/torvalds/linux/commit/8486d357129126e27fb5d4e02622566bafc77035) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-19 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 42 |
| 编译错误数（gcc syntax-only） | 13（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #8486d357129126e27fb5d4e02622566bafc77035 (https://github.com/torvalds/linux/commit/8486d357129126e27fb5d4e02622566bafc77035)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 1581；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 8486d357129126e27fb5d4e02622566bafc77035 Merge tag 'spi-fix-v7.3-rc7' of git://git.kernel.org/pub/scm/linux/kernel/git/broonie/spi :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1578,11 +1578,13 @@ static int spi_imx_dma_tx_data_handle(struct spi_imx_data *spi_imx,
 		dma_data->dma_len = ALIGN(dma_data->data_len, BYTES_PER_32BITS_WORD);
 	}
 
-	dma_data->dma_tx_buf = kzalloc(dma_data->dma_len, GFP_KERNEL);
+	dma_data->dma_tx_buf = kzalloc(DMA_CACHE_ALIGNED_LEN(dma_data->dma_len),
+				       GFP_KERNEL);
 	if (!dma_data->dma_tx_buf)
 		return -ENOMEM;
 
-	dma_data->dma_rx_buf = kzalloc(dma_data->dma_len, GFP_KERNEL);
+	dma_data->dma_rx_buf = kzalloc(DMA_CACHE_ALIGNED_LEN(dma_data->dma_len),
+				       GFP_KERNEL);
 	if (!dma_data->dma_rx_buf) {
 		kfree(dma_data->dma_tx_buf);
 		return -ENOMEM;
@@ -1920,7 +1922,8 @@ static int spi_imx_dma_package_transfer(struct spi_imx_data *spi_imx,
 
 	/* Trim the DMA RX buffer and copy the actual data to rx_buf */
 	dma_sync_single_for_cpu(controller->dma_rx->device->dev, dma_data->dma_rx_addr,
-				dma_data->dma_len, DMA_FROM_DEVICE);
+				DMA_CACHE_ALIGNED_LEN(dma_data->dma_len),
+				DMA_FROM_DEVICE);
 	spi_imx_dma_rx_data_handle(spi_imx, dma_data, transfer->rx_buf + spi_imx->rx_offset,
 				   word_delay);
 	spi_imx->rx_offset += dma_data->data_len;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`dma_sync_single_for_cpu`
- 外部函数：`int`
- 外部函数：`kfree`
- 外部函数：`kzalloc`
- 外部函数：`memcpy`
- 外部函数：`swab32s`
- 外部函数：`swahw32s`
- 外部函数：`void`
- 大写宏：`ALIGN`
- 大写宏：`CSPI`
- 大写宏：`DIV_ROUND_UP`
- 大写宏：`DMA`
- 大写宏：`DMA_FROM_DEVICE`
- 大写宏：`ECSPI`
- 大写宏：`ENOMEM`
- 大写宏：`ERR009165`
- 大写宏：`FIFO`
- 大写宏：`GFP_KERNEL`
- 大写宏：`IMX1_CSPI`
- 大写宏：`IMX21_CSPI`
- 大写宏：`IMX27_CSPI`
- 大写宏：`IMX31_CSPI`
- 大写宏：`IMX35_CSPI`
- 大写宏：`IMX51_ECSPI`
- 大写宏：`IMX53_ECSPI`
- 大写宏：`IMX6DQCE`
- 大写宏：`LSB`
- 大写宏：`RXFIFO`
- 大写宏：`SPI`
- 外部类型：`CPUs`
- 外部类型：`On`
- 外部类型：`So`
- 外部类型：`Swap`
- 外部类型：`Target`
- 外部类型：`This`
- 外部类型：`Trim`
- 外部类型：`When`
- 外部类型：`clk`
- 外部类型：`completion`
- 外部类型：`device`
- 外部类型：`dma_addr_t`
- 外部类型：`size_t`
- 外部类型：`spi_controller`
- 外部类型：`spi_device`
- 外部类型：`spi_message`
- 外部类型：`spi_transfer`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-6af26b8934` → 本草稿移入 `cases/defect/auto-linux-6af26b8934/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
