# auto-linux-cabb29cafc

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#045958acaa75205b31578cb22c4345e28dd2e4bf](https://github.com/torvalds/linux/commit/045958acaa75205b31578cb22c4345e28dd2e4bf) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-16 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 145 |
| 编译错误数（gcc syntax-only） | 197（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #045958acaa75205b31578cb22c4345e28dd2e4bf (https://github.com/torvalds/linux/commit/045958acaa75205b31578cb22c4345e28dd2e4bf)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 1566；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 045958acaa75205b31578cb22c4345e28dd2e4bf Merge tag 'net-7.3-rc8' of git://git.kernel.org/pub/scm/linux/kernel/git/netdev/net :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1552,6 +1552,8 @@ static int owl_emac_probe(struct platform_device *pdev)
 		return ret;
 	}
 
+	netdev->netdev_ops = &owl_emac_netdev_ops;
+
 	ret = owl_emac_phy_init(netdev);
 	if (ret) {
 		dev_err(dev, "failed to initialize PHY\n");
@@ -1563,7 +1565,6 @@ static int owl_emac_probe(struct platform_device *pdev)
 	netdev->min_mtu = OWL_EMAC_MTU_MIN;
 	netdev->max_mtu = OWL_EMAC_MTU_MAX;
 	netdev->watchdog_timeo = OWL_EMAC_TX_TIMEOUT;
-	netdev->netdev_ops = &owl_emac_netdev_ops;
 	netdev->ethtool_ops = &owl_emac_ethtool_ops;
 	netif_napi_add(netdev, &priv->napi, owl_emac_poll);
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`container_of`
- 外部函数：`dev_dbg_ratelimited`
- 外部函数：`dev_err`
- 外部函数：`dev_kfree_skb`
- 外部函数：`dma_map_single`
- 外部函数：`dma_mapping_error`
- 外部函数：`dma_rmb`
- 外部函数：`dma_unmap_single`
- 外部函数：`dma_wmb`
- 外部函数：`eth_type_trans`
- 外部函数：`napi_complete_done`
- 外部函数：`netdev_alloc_skb`
- 外部函数：`netdev_dbg`
- 外部函数：`netdev_err`
- 外部函数：`netdev_priv`
- 外部函数：`netif_msg_link`
- 外部函数：`netif_napi_add`
- 外部函数：`netif_queue_stopped`
- 外部函数：`netif_receive_skb`
- 外部函数：`netif_wake_queue`
- 外部函数：`of_phy_get_and_connect`
- 外部函数：`phy_attached_info`
- 外部函数：`phy_print_status`
- 外部函数：`phy_set_sym_pause`
- 外部函数：`readl`
- 外部函数：`schedule_work`
- 外部函数：`skb_headlen`
- 外部函数：`skb_put`
- 外部函数：`skb_reserve`
- 外部函数：`skb_tail_pointer`
- 外部函数：`skb_tailroom`
- 外部函数：`spin_lock`
- 外部函数：`spin_lock_bh`
- 外部函数：`spin_unlock`
- 外部函数：`spin_unlock_bh`
- 外部函数：`unlikely`
- 外部函数：`writel`
- 大写宏：`AHB`
- 大写宏：`AIE`
- 大写宏：`CHECKSUM_NONE`
- 大写宏：`CIRC_SPACE`
- 大写宏：`CMD`
- 大写宏：`DMA`
- 大写宏：`DMA_FROM_DEVICE`
- 大写宏：`DMA_TO_DEVICE`
- 大写宏：`DUPLEX_FULL`
- 大写宏：`ENODEV`
- 大写宏：`ETH_FCS_LEN`
- 大写宏：`ETI`
- 大写宏：`FIXME`
- 大写宏：`MAC`
- 大写宏：`NIE`
- 大写宏：`NULL`
- 大写宏：`OWL_EMAC_BIT_MAC_CSR20_BPE`
- 大写宏：`OWL_EMAC_BIT_MAC_CSR20_FCE`
- 大写宏：`OWL_EMAC_BIT_MAC_CSR20_RPE`
- 大写宏：`OWL_EMAC_BIT_MAC_CSR20_TPE`
- 大写宏：`OWL_EMAC_BIT_MAC_CSR20_TUE`
- 大写宏：`OWL_EMAC_BIT_MAC_CSR5_AIS`
- 大写宏：`OWL_EMAC_BIT_MAC_CSR5_ETI`
- 大写宏：`OWL_EMAC_BIT_MAC_CSR5_NIS`
- 大写宏：`OWL_EMAC_BIT_MAC_CSR5_RI`
- 大写宏：`OWL_EMAC_BIT_MAC_CSR5_RU`
- 大写宏：`OWL_EMAC_BIT_MAC_CSR5_TI`
- 大写宏：`OWL_EMAC_BIT_MAC_CSR6_FD`
- 大写宏：`OWL_EMAC_BIT_MAC_CSR7_AIE`
- 大写宏：`OWL_EMAC_BIT_MAC_CSR7_ALL_NOT_TUE`
- 大写宏：`OWL_EMAC_BIT_MAC_CSR7_NIE`
- 大写宏：`OWL_EMAC_BIT_RDES0_CE`
- 大写宏：`OWL_EMAC_BIT_RDES0_CS`
- 大写宏：`OWL_EMAC_BIT_RDES0_DB`
- 大写宏：`OWL_EMAC_BIT_RDES0_DE`
- 大写宏：`OWL_EMAC_BIT_RDES0_OWN`
- 大写宏：`OWL_EMAC_BIT_RDES0_RF`
- 大写宏：`OWL_EMAC_BIT_RDES0_TL`
- 大写宏：`OWL_EMAC_BIT_RDES0_ZERO`
- 大写宏：`OWL_EMAC_BIT_TDES0_DE`
- 大写宏：`OWL_EMAC_BIT_TDES0_EC`
- 大写宏：`OWL_EMAC_BIT_TDES0_ES`
- 大写宏：`OWL_EMAC_BIT_TDES0_LC`
- 大写宏：`OWL_EMAC_BIT_TDES0_LO`
- 大写宏：`OWL_EMAC_BIT_TDES0_NC`
- 大写宏：`OWL_EMAC_BIT_TDES0_OWN`
- 大写宏：`OWL_EMAC_BIT_TDES0_UF`
- 大写宏：`OWL_EMAC_MSK_MAC_CSR5_RS`
- 大写宏：`OWL_EMAC_MSK_MAC_CSR5_TS`
- 大写宏：`OWL_EMAC_MSK_MAC_CSR6_SPEED`
- 大写宏：`OWL_EMAC_MSK_MAC_CSR6_STSR`
- 大写宏：`OWL_EMAC_MSK_RDES0_FL`
- 大写宏：`OWL_EMAC_MTU_MAX`
- 大写宏：`OWL_EMAC_MTU_MIN`
- 大写宏：`OWL_EMAC_OFF_MAC_CSR5_RS`
- 大写宏：`OWL_EMAC_OFF_MAC_CSR5_TS`
- 大写宏：`OWL_EMAC_OFF_MAC_CSR6_SPEED`
- 大写宏：`OWL_EMAC_OFF_RDES0_FL`
- 大写宏：`OWL_EMAC_REG_MAC_CSR2`
- 大写宏：`OWL_EMAC_REG_MAC_CSR20`
- 大写宏：`OWL_EMAC_REG_MAC_CSR5`
- 大写宏：`OWL_EMAC_REG_MAC_CSR6`
- 大写宏：`OWL_EMAC_REG_MAC_CSR7`
- 大写宏：`OWL_EMAC_RX_FRAME_MAX_LEN`
- 大写宏：`OWL_EMAC_SKB_ALIGN`
- 大写宏：`OWL_EMAC_SKB_RESERVE`
- 大写宏：`OWL_EMAC_TX_TIMEOUT`
- 大写宏：`OWL_EMAC_VAL_MAC_CSR2_RPD`
- 大写宏：`OWL_EMAC_VAL_MAC_CSR5_RS_CDES`
- 大写宏：`OWL_EMAC_VAL_MAC_CSR5_RS_DATA`
- 大写宏：`OWL_EMAC_VAL_MAC_CSR5_RS_FDES`
- 大写宏：`OWL_EMAC_VAL_MAC_CSR5_TS_CDES`
- 大写宏：`OWL_EMAC_VAL_MAC_CSR5_TS_DATA`
- 大写宏：`OWL_EMAC_VAL_MAC_CSR6_SPEED_100M`
- 大写宏：`OWL_EMAC_VAL_MAC_CSR6_SPEED_10M`
- 大写宏：`OWN`
- 大写宏：`PHY`
- 大写宏：`READ_ONCE`
- 大写宏：`SPEED_100`
- 大写宏：`WRITE_ONCE`
- 外部类型：`At`
- 外部类型：`Buffer`
- 外部类型：`But`
- 外部类型：`Check`
- 外部类型：`Count`
- 外部类型：`Enable`
- 外部类型：`Ensure`
- 外部类型：`Flush`
- 外部类型：`Found`
- 外部类型：`Guard`
- 外部类型：`If`
- 外部类型：`Note`
- 外部类型：`Prepare`
- 外部类型：`Reset`
- 外部类型：`Restore`
- 外部类型：`Reuse`
- 外部类型：`Some`
- 外部类型：`Temporarily`
- 外部类型：`This`
- 外部类型：`Update`
- 外部类型：`device`
- 外部类型：`dma_addr_t`
- 外部类型：`napi_struct`
- 外部类型：`net_device`
- 外部类型：`owl_emac_priv`
- 外部类型：`owl_emac_ring`
- 外部类型：`owl_emac_ring_desc`
- 外部类型：`phy_device`
- 外部类型：`sk_buff`
- 外部类型：`uintptr_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-cabb29cafc` → 本草稿移入 `cases/defect/auto-linux-cabb29cafc/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
