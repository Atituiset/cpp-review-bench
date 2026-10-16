# auto-linux-82b759b6a3

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
| 外部依赖数（dep_count） | 55 |
| 编译错误数（gcc syntax-only） | 96（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #045958acaa75205b31578cb22c4345e28dd2e4bf (https://github.com/torvalds/linux/commit/045958acaa75205b31578cb22c4345e28dd2e4bf)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 1501；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 045958acaa75205b31578cb22c4345e28dd2e4bf Merge tag 'net-7.3-rc8' of git://git.kernel.org/pub/scm/linux/kernel/git/netdev/net :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1421,6 +1421,8 @@ static int greth_of_probe(struct platform_device *ofdev)
 	/* Check if MAC can handle MDIO interrupts */
 	greth->mdio_int_en = (tmp >> 26) & 1;
 
+	dev->netdev_ops = &greth_netdev_ops;
+
 	err = greth_mdio_init(greth);
 	if (err) {
 		if (netif_msg_probe(greth))
@@ -1498,7 +1500,6 @@ static int greth_of_probe(struct platform_device *ofdev)
 		dev->flags &= ~IFF_MULTICAST;
 	}
 
-	dev->netdev_ops = &greth_netdev_ops;
 	dev->ethtool_ops = &greth_ethtool_ops;
 
 	err = register_netdev(dev);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__raw_readl`
- 外部函数：`__raw_writel`
- 外部函数：`cpu_to_be32`
- 外部函数：`dev_err`
- 外部函数：`linkmode_copy`
- 外部函数：`mdiobus_alloc`
- 外部函数：`mdiobus_free`
- 外部函数：`mdiobus_register`
- 外部函数：`mdiobus_unregister`
- 外部函数：`netdev_priv`
- 外部函数：`netif_msg_ifup`
- 外部函数：`netif_msg_probe`
- 外部函数：`phy_aneg_done`
- 外部函数：`phy_connect_direct`
- 外部函数：`phy_find_first`
- 外部函数：`phy_read_status`
- 外部函数：`phy_set_max_speed`
- 外部函数：`phy_start`
- 外部函数：`phy_start_aneg`
- 外部函数：`pr_debug`
- 外部函数：`register_netdev`
- 外部函数：`snprintf`
- 外部函数：`spin_lock_irqsave`
- 外部函数：`spin_unlock_irqrestore`
- 外部函数：`time_after`
- 外部函数：`time_before`
- 外部函数：`up`
- 外部函数：`wait_for_mdio`
- 大写宏：`DUPLEX_FULL`
- 大写宏：`EBUSY`
- 大写宏：`ENOMEM`
- 大写宏：`ENXIO`
- 大写宏：`GRETH_CTRL_FD`
- 大写宏：`GRETH_CTRL_GB`
- 大写宏：`GRETH_CTRL_SP`
- 大写宏：`GRETH_MII_BUSY`
- 大写宏：`GRETH_MII_NVALID`
- 大写宏：`IFF_MULTICAST`
- 大写宏：`MAC`
- 大写宏：`MDIO`
- 大写宏：`MII_BUS_ID_SIZE`
- 大写宏：`NULL`
- 大写宏：`PHY`
- 大写宏：`PHY_INTERFACE_MODE_GMII`
- 大写宏：`PHY_INTERFACE_MODE_MII`
- 大写宏：`SPEED_100`
- 大写宏：`SPEED_1000`
- 外部类型：`Check`
- 外部类型：`Ethernet`
- 外部类型：`Find`
- 外部类型：`Full`
- 外部类型：`Half`
- 外部类型：`If`
- 外部类型：`greth_private`
- 外部类型：`mii_bus`
- 外部类型：`net_device`
- 外部类型：`phy_device`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-82b759b6a3` → 本草稿移入 `cases/defect/auto-linux-82b759b6a3/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
