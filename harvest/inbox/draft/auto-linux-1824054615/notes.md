# auto-linux-1824054615

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
| 外部依赖数（dep_count） | 50 |
| 编译错误数（gcc syntax-only） | 46（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #045958acaa75205b31578cb22c4345e28dd2e4bf (https://github.com/torvalds/linux/commit/045958acaa75205b31578cb22c4345e28dd2e4bf)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 751；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 045958acaa75205b31578cb22c4345e28dd2e4bf Merge tag 'net-7.3-rc8' of git://git.kernel.org/pub/scm/linux/kernel/git/netdev/net :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -733,6 +733,8 @@ static int adin1140_probe(struct spi_device *spi)
 	if (ret)
 		return ret;
 
+	netdev->netdev_ops = &adin1140_netdev_ops;
+
 	ret = adin1140_phy_init(priv, spi);
 	if (ret)
 		return ret;
@@ -748,7 +750,6 @@ static int adin1140_probe(struct spi_device *spi)
 
 	netdev->if_port = IF_PORT_10BASET;
 	netdev->irq = spi->irq;
-	netdev->netdev_ops = &adin1140_netdev_ops;
 	netdev->ethtool_ops = &adin1140_ethtool_ops;
 	netdev->netns_immutable = true;
 	netdev->priv_flags |= IFF_LIVE_ADDR_CHANGE |
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`dev_err_probe`
- 外部函数：`dev_name`
- 外部函数：`devm_add_action_or_reset`
- 外部函数：`devm_mdiobus_alloc`
- 外部函数：`devm_mdiobus_register`
- 外部函数：`phy_attached_info`
- 外部函数：`phy_connect_direct`
- 外部函数：`phy_disconnect`
- 外部函数：`phy_find_first`
- 外部函数：`phy_print_status`
- 外部函数：`register`
- 外部函数：`reset`
- 外部函数：`snprintf`
- 外部函数：`spi_get_chipselect`
- 大写宏：`ADIN1140`
- 大写宏：`ADIN1140_STATS_CNT`
- 大写宏：`C22`
- 大写宏：`C45`
- 大写宏：`CONFIG0`
- 大写宏：`CONFIG2`
- 大写宏：`EIO`
- 大写宏：`ENODEV`
- 大写宏：`ENOMEM`
- 大写宏：`GENMASK`
- 大写宏：`IFF_LIVE_ADDR_CHANGE`
- 大写宏：`IF_PORT_10BASET`
- 大写宏：`MDIO`
- 大写宏：`MII_BMCR`
- 大写宏：`MII_BMSR`
- 大写宏：`MII_BUS_ID_SIZE`
- 大写宏：`MII_PHYSID1`
- 大写宏：`MII_PHYSID2`
- 大写宏：`PHY`
- 大写宏：`PHY_INTERFACE_MODE_INTERNAL`
- 大写宏：`T1S`
- 大写宏：`TC6`
- 外部类型：`Both`
- 外部类型：`Can`
- 外部类型：`In`
- 外部类型：`No`
- 外部类型：`Protects`
- 外部类型：`Since`
- 外部类型：`The`
- 外部类型：`These`
- 外部类型：`This`
- 外部类型：`delayed_work`
- 外部类型：`mii_bus`
- 外部类型：`net_device`
- 外部类型：`oa_tc6`
- 外部类型：`phy_device`
- 外部类型：`spi_device`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-1824054615` → 本草稿移入 `cases/defect/auto-linux-1824054615/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
