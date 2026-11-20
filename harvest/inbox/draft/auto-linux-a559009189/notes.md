# auto-linux-a559009189

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#1e575bda30acc0bf01f799cee798d4a1c100d538](https://github.com/torvalds/linux/commit/1e575bda30acc0bf01f799cee798d4a1c100d538) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-20 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 131 |
| 编译错误数（gcc syntax-only） | 132（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #1e575bda30acc0bf01f799cee798d4a1c100d538 (https://github.com/torvalds/linux/commit/1e575bda30acc0bf01f799cee798d4a1c100d538)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 1762；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 1e575bda30acc0bf01f799cee798d4a1c100d538 Merge tag 'net-7.4-rc4' of git://git.kernel.org/pub/scm/linux/kernel/git/netdev/net :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1132,6 +1132,7 @@ static void qca_controller_memdump(struct work_struct *work)
 		if (!test_bit(QCA_MEMDUMP_COLLECTION, &qca->flags)) {
 			bt_dev_err(hu->hdev, "QCA: Discarding other packets");
 			kfree(qca_memdump);
+			qca->qca_memdump = NULL;
 			kfree_skb(skb);
 			mutex_unlock(&qca->hci_memdump_lock);
 			return;
@@ -1386,6 +1387,7 @@ static int qca_set_baudrate(struct hci_dev *hdev, uint8_t baudrate)
 	case QCA_WCN3991:
 	case QCA_WCN3998:
 	case QCA_WCN6750:
+	case QCA_QCA2066:
 	case QCA_WCN6855:
 	case QCA_WCN7850:
 		usleep_range(1000, 10000);
@@ -1473,6 +1475,7 @@ static int qca_check_speeds(struct hci_uart *hu)
 	case QCA_WCN3991:
 	case QCA_WCN3998:
 	case QCA_WCN6750:
+	case QCA_QCA2066:
 	case QCA_WCN6855:
 	case QCA_WCN7850:
 		if (!qca_get_speed(hu, QCA_INIT_SPEED) &&
@@ -1516,6 +1519,7 @@ static int qca_set_speed(struct hci_uart *hu, enum qca_speed_type speed_type)
 		case QCA_WCN3991:
 		case QCA_WCN3998:
 		case QCA_WCN6750:
+		case QCA_QCA2066:
 		case QCA_WCN6855:
 		case QCA_WCN7850:
 			hci_uart_set_flow_control(hu, true);
@@ -1551,6 +1555,7 @@ static int qca_set_speed(struct hci_uart *hu, enum qca_speed_type speed_type)
 		case QCA_WCN3991:
 		case QCA_WCN3998:
 		case QCA_WCN6750:
+		case QCA_QCA2066:
 		case QCA_WCN6855:
 		case QCA_WCN7850:
 			hci_uart_set_flow_control(hu, false);
@@ -1750,6 +1755,27 @@ static bool qca_wakeup(struct hci_dev *hdev)
 	return wakeup;
 }
 
+static void qca_port_close(struct hci_uart *hu)
+{
+	/* HCI_UART_PROTO_READY tracks whether the port is open, so a port
+	 * left closed by a failed qca_port_open() is not closed again by
+	 * hci_uart_close() or hci_uart_unregister_device().
+	 */
+	if (test_and_clear_bit(HCI_UART_PROTO_READY, &hu->flags))
+		serdev_device_close(hu->serdev);
+}
+
+static int qca_port_open(struct hci_uart *hu)
+{
+	int ret;
+
+	ret = serdev_device_open(hu->serdev);
+	if (!ret)
+		set_bit(HCI_UART_PROTO_READY, &hu->flags);
+
+	return ret;
+}
+
 static int qca_port_reopen(struct hci_uart *hu)
 {
 	int ret;
@@ -1759,8 +1785,8 @@ static int qca_port_reopen(struct hci_uart *hu)
 	 * Without this, we will have RTS and CTS synchronization
 	 * issues.
 	 */
-	serdev_device_close(hu->serdev);
-	ret = serdev_device_open(hu->serdev);
+	qca_port_close(hu);
+	ret = qca_port_open(hu);
 	if (ret) {
 		bt_dev_err(hu->hdev, "failed to open port");
 		return ret;
@@ -1784,12 +1810,12 @@ static int qca_regulator_init(struct hci_uart *hu)
 	qcadev = serdev_device_get_drvdata(hu->serdev);
 
 	if (!qcadev->bt_power->vregs_on) {
-		serdev_device_close(hu->serdev);
+		qca_port_close(hu);
 		ret = qca_regulator_enable(qcadev);
 		if (ret)
 			return ret;
 
-		ret = serdev_device_open(hu->serdev);
+		ret = qca_port_open(hu);
 		if (ret) {
 			bt_dev_err(hu->hdev, "failed to open port");
 			return ret;
@@ -1867,6 +1893,7 @@ static int qca_power_on(struct hci_dev *hdev)
 	case QCA_WCN3991:
 	case QCA_WCN3998:
 	case QCA_WCN6750:
+	case QCA_QCA2066:
 	case QCA_WCN6855:
 	case QCA_WCN7850:
 		ret = qca_regulator_init(hu);
@@ -1989,6 +2016,7 @@ static int qca_setup(struct hci_uart *hu)
 	case QCA_WCN3991:
 	case QCA_WCN3998:
 	case QCA_WCN6750:
+	case QCA_QCA2066:
 	case QCA_WCN6855:
 	case QCA_WCN7850:
 		if (qcadev && qcadev->bdaddr_property_broken)
@@ -2022,6 +2050,7 @@ static int qca_setup(struct hci_uart *hu)
 	case QCA_WCN3991:
 	case QCA_WCN3998:
 	case QCA_WCN6750:
+	case QCA_QCA2066:
 	case QCA_WCN6855:
 	case QCA_WCN7850:
 		break;
@@ -2065,8 +2094,8 @@ static int qca_setup(struct hci_uart *hu)
 		if (retries < MAX_INIT_RETRIES) {
 			bt_dev_warn(hdev, "Retry BT power ON:%d", retries);
 			if (hu->serdev) {
-				serdev_device_close(hu->serdev);
-				ret = serdev_device_open(hu->serdev);
+				qca_port_close(hu);
+				ret = qca_port_open(hu);
 				if (ret) {
 					bt_dev_err(hdev, "failed to open port");
 					return ret;
@@ -2284,6 +2313,7 @@ static void qca_power_off(struct hci_uart *hu)
 		break;
 
 	case QCA_WCN6750:
+	case QCA_QCA2066:
 	case QCA_WCN6855:
 		
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`bt_dev_dbg`
- 外部函数：`bt_dev_err`
- 外部函数：`bt_dev_warn`
- 外部函数：`bt_skb_alloc`
- 外部函数：`clear_bit`
- 外部函数：`clk_disable_unprepare`
- 外部函数：`clk_prepare_enable`
- 外部函数：`dev_err`
- 外部函数：`device_property_present`
- 外部函数：`gpiod_get_value_cansleep`
- 外部函数：`gpiod_set_value_cansleep`
- 外部函数：`hci_get_drvdata`
- 外部函数：`hci_skb_pkt_type`
- 外部函数：`hci_uart_set_baudrate`
- 外部函数：`hci_uart_set_flow_control`
- 外部函数：`hci_uart_tx_wakeup`
- 外部函数：`kfree`
- 外部函数：`kfree_skb`
- 外部函数：`msecs_to_jiffies`
- 外部函数：`msleep`
- 外部函数：`mutex_unlock`
- 外部函数：`pwrseq_enable`
- 外部函数：`qca_set_speed`
- 外部函数：`regulator_bulk_disable`
- 外部函数：`regulator_bulk_enable`
- 外部函数：`reinit_completion`
- 外部函数：`serdev_device_close`
- 外部函数：`serdev_device_get_drvdata`
- 外部函数：`serdev_device_open`
- 外部函数：`serdev_device_set_baudrate`
- 外部函数：`serdev_device_wait_until_sent`
- 外部函数：`serdev_device_write_buf`
- 外部函数：`serdev_device_write_flush`
- 外部函数：`set_bit`
- 外部函数：`skb_put_data`
- 外部函数：`skb_queue_empty`
- 外部函数：`skb_queue_tail`
- 外部函数：`test_bit`
- 外部函数：`usleep_range`
- 外部函数：`wait_for_completion_timeout`
- 大写宏：`BT_DBG`
- 大写宏：`COEX`
- 大写宏：`CTS`
- 大写宏：`EINVAL`
- 大写宏：`ENOMEM`
- 大写宏：`ETIMEDOUT`
- 大写宏：`GFP_KERNEL`
- 大写宏：`GPIOD_IN`
- 大写宏：`HCI_COMMAND_PKT`
- 大写宏：`HCI_IBS`
- 大写宏：`IS_ERR`
- 大写宏：`QCA`
- 大写宏：`QCA_BAUDRATE_1000000`
- 大写宏：`QCA_BAUDRATE_115200`
- 大写宏：`QCA_BAUDRATE_19200`
- 大写宏：`QCA_BAUDRATE_2000000`
- 大写宏：`QCA_BAUDRATE_230400`
- 大写宏：`QCA_BAUDRATE_3000000`
- 大写宏：`QCA_BAUDRATE_3200000`
- 大写宏：`QCA_BAUDRATE_3500000`
- 大写宏：`QCA_BAUDRATE_38400`
- 大写宏：`QCA_BAUDRATE_460800`
- 大写宏：`QCA_BAUDRATE_500000`
- 大写宏：`QCA_BAUDRATE_57600`
- 大写宏：`QCA_BAUDRATE_921600`
- 大写宏：`QCA_BAUDRATE_9600`
- 大写宏：`QCA_DROP_VENDOR_EVENT`
- 大写宏：`QCA_INIT_SPEED`
- 大写宏：`QCA_MEMDUMP_COLLECTED`
- 大写宏：`QCA_MEMDUMP_COLLECTING`
- 大写宏：`QCA_MEMDUMP_COLLECTION`
- 大写宏：`QCA_MEMDUMP_IDLE`
- 大写宏：`QCA_MEMDUMP_TIMEOUT`
- 大写宏：`QCA_OPER_SPEED`
- 大写宏：`QCA_ROME`
- 大写宏：`QCA_WCN3950`
- 大写宏：`QCA_WCN3988`
- 大写宏：`QCA_WCN3990`
- 大写宏：`QCA_WCN3990_POWEROFF_PULSE`
- 大写宏：`QCA_WCN3990_POWERON_PULSE`
- 大写宏：`QCA_WCN3991`
- 大写宏：`QCA_WCN3998`
- 大写宏：`QCA_WCN6750`
- 大写宏：`QCA_WCN6855`
- 大写宏：`QCA_WCN7850`
- 大写宏：`RTS`
- 大写宏：`SW_CTRL`
- 大写宏：`UART`
- 外部类型：`Already`
- 外部类型：`Assign`
- 外部类型：`Check`
- 外部类型：`Clock`
- 外部类型：`Disable`
- 外部类型：`Disabling`
- 外部类型：`Discarding`
- 外部类型：`Failed`
- 外部类型：`Fi`
- 外部类型：`For`
- 外部类型：`Forcefully`
- 外部类型：`Give`
- 外部类型：`Memdump`
- 外部类型：`On`
- 外部类型：`Powering`
- 外部类型：`Retry`
- 外部类型：`Set`
- 外部类型：`SoC`
- 外部类型：`These`
- 外部类型：`This`
- 外部类型：`Tx`
- 外部类型：`Wait`
- 外部类型：`Wi`
- 外部类型：`Without`
- 外部类型：`clk`
- 外部类型：`completion`
- 外部类型：`delayed_work`
- 外部类型：`dentry`
- 外部类型：`device`
- 外部类型：`gpio_desc`
- 外部类型：`hci_dev`
- 外部类型：`hci_uart`
- 外部类型：`mutex`
- 外部类型：`pwrseq_desc`
- 外部类型：`qca_btsoc_type`
- 外部类型：`regulator_bulk_data`
- 外部类型：`sk_buff`
- 外部类型：`sk_buff_head`
- 外部类型：`spinlock_t`
- 外部类型：`timer_list`
- 外部类型：`uint8_t`
- 外部类型：`wait_queue_head_t`
- 外部类型：`work_struct`
- 外部类型：`workqueue_struct`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-a559009189` → 本草稿移入 `cases/defect/auto-linux-a559009189/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
