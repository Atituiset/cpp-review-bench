# auto-linux-36060cc717

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#f66469d9905b39d2c260a15d7d6db450f4db67c1](https://github.com/torvalds/linux/commit/f66469d9905b39d2c260a15d7d6db450f4db67c1) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-06 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 61 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #f66469d9905b39d2c260a15d7d6db450f4db67c1 (https://github.com/torvalds/linux/commit/f66469d9905b39d2c260a15d7d6db450f4db67c1)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 1568；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT f66469d9905b39d2c260a15d7d6db450f4db67c1 Merge tag 'net-7.4-rc2' of git://git.kernel.org/pub/scm/linux/kernel/git/netdev/net :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -7,6 +7,7 @@
 #include <linux/usb.h>
 #include <linux/iopoll.h>
 #include <linux/overflow.h>
+#include <linux/string.h>
 #include <linux/unaligned.h>
 
 #include <net/bluetooth/bluetooth.h>
@@ -30,7 +31,7 @@
 #define BTMTK_CBMCU_FLAG_ENABLE_PATCH	0xF1
 
 /* It is for mt79xx iso data transmission setting */
-#define MTK_ISO_THRESHOLD	264
+#define MTK_ISO_THRESHOLD	342
 
 struct btmtk_patch_header {
 	u8 datetime[16];
@@ -1556,16 +1557,32 @@ static int btmtk_recv_isopkt(struct hci_dev *hdev, void *buffer, int count)
 	struct btmtk_data *btmtk_data = hci_get_priv(hdev);
 	struct sk_buff *skb;
 	unsigned long flags;
+	unsigned int iso_mtu;
 	int err = 0;
 
+	/* The controller reports the largest Data_Total_Length it will send in
+	 * LE Read Buffer Size [v2]. Fall back to HCI_MAX_ISO_SIZE while that
+	 * command has not completed yet.
+	 */
+	iso_mtu = hdev->iso_mtu ? hdev->iso_mtu : HCI_MAX_ISO_SIZE;
+
 	spin_lock_irqsave(&btmtk_data->isorxlock, flags);
 	skb = btmtk_data->isopkt_skb;
 
 	while (count) {
 		int len;
 
 		if (!skb) {
-			skb = bt_skb_alloc(HCI_MAX_ISO_SIZE, GFP_ATOMIC);
+			/* Drop the endpoint's zero-filled padding before
+			 * allocating an skb for it; a zero Data_Total_Length
+			 * check below would miss a padding remainder
+			 * shorter than an ISO header.
+			 */
+			if (mem_is_zero(buffer, count))
+				break;
+
+			skb = bt_skb_alloc(HCI_ISO_HDR_SIZE + iso_mtu,
+					   GFP_ATOMIC);
 			if (!skb) {
 				err = -ENOMEM;
 				break;
@@ -1588,7 +1605,24 @@ static int btmtk_recv_isopkt(struct hci_dev *hdev, void *buffer, int count)
 			/* Complete ISO header */
 			hci_skb_expect(skb) = __le16_to_cpu(dlen);
 
-			if (skb_tailroom(skb) < hci_skb_expect(skb)) {
+			if (!hci_skb_expect(skb)) {
+				/* Zero padding was ruled out above, so a
+				 * zero Data_Total_Length here is invalid.
+				 */
+				kfree_skb(skb);
+				skb = NULL;
+
+				err = -EILSEQ;
+				break;
+			}
+
+			/* Reject a length the controller said it would never
+			 * send. The skb_tailroom() test is kept because a
+			 * fragment started before hdev->iso_mtu was populated
+			 * sits in a smaller skb than iso_mtu now allows.
+			 */
+			if (hci_skb_expect(skb) > iso_mtu ||
+			    skb_tailroom(skb) < hci_skb_expect(skb)) {
 				kfree_skb(skb);
 				skb = NULL;
 
@@ -1707,13 +1741,15 @@ static int btmtk_submit_intr_urb(struct hci_dev *hdev, gfp_t mem_flags)
 static int btmtk_usb_isointf_init(struct hci_dev *hdev)
 {
 	struct btmtk_data *btmtk_data = hci_get_priv(hdev);
-	u8 iso_param[2] = { 0x08, 0x01 };
+	__le16 iso_param = cpu_to_le16(MTK_ISO_THRESHOLD);
 	struct sk_buff *skb;
 	int err;
 
 	spin_lock_init(&btmtk_data->isorxlock);
 
-	__set_mtk_intr_interface(hdev);
+	err = __set_mtk_intr_interface(hdev);
+	if (err < 0)
+		return err;
 
 	err = btmtk_submit_intr_urb(hdev, GFP_KERNEL);
 	if (err < 0) {
@@ -1722,9 +1758,10 @@ static int btmtk_usb_isointf_init(struct hci_dev *hdev)
 		return err;
 	}
 
-	skb = __hci_cmd_sync(hdev, 0xfd98, sizeof(iso_param), iso_param,
+	skb = __hci_cmd_sync(hdev, 0xfd98, sizeof(iso_param), &iso_param,
 			     HCI_INIT_TIMEOUT);
 	if (IS_ERR(skb)) {
+		usb_kill_anchored_urbs(&btmtk_data->isopkt_anchor);
 		bt_dev_err(hdev, "Failed to apply iso setting (%ld)", PTR_ERR(skb));
 		return PTR_ERR(skb);
 	}
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__hci_cmd_sync`
- 外部函数：`__le16_to_cpu`
- 外部函数：`bt_dev_err`
- 外部函数：`bt_skb_alloc`
- 外部函数：`failed`
- 外部函数：`hci_cmd_sync_cancel`
- 外部函数：`hci_get_priv`
- 外部函数：`hci_recv_frame`
- 外部函数：`hci_skb_expect`
- 外部函数：`hci_skb_pkt_type`
- 外部函数：`kfree_skb`
- 外部函数：`kmalloc`
- 外部函数：`le16_to_cpu`
- 外部函数：`min_t`
- 外部函数：`resubmit`
- 外部函数：`setting`
- 外部函数：`skb_put_data`
- 外部函数：`skb_tailroom`
- 外部函数：`spin_lock_init`
- 外部函数：`spin_lock_irqsave`
- 外部函数：`spin_unlock_irqrestore`
- 外部函数：`test_bit`
- 外部函数：`usb_alloc_urb`
- 外部函数：`usb_anchor_urb`
- 外部函数：`usb_fill_int_urb`
- 外部函数：`usb_find_common_endpoints`
- 外部函数：`usb_free_urb`
- 外部函数：`usb_mark_last_busy`
- 外部函数：`usb_rcvintpipe`
- 外部函数：`usb_set_interface`
- 外部函数：`usb_submit_urb`
- 外部函数：`usb_unanchor_urb`
- 大写宏：`BT_DBG`
- 大写宏：`EILSEQ`
- 大写宏：`ENODEV`
- 大写宏：`ENOENT`
- 大写宏：`ENOMEM`
- 大写宏：`EPERM`
- 大写宏：`GFP_ATOMIC`
- 大写宏：`GFP_KERNEL`
- 大写宏：`HCI_INIT_TIMEOUT`
- 大写宏：`HCI_ISODATA_PKT`
- 大写宏：`HCI_ISO_HDR_SIZE`
- 大写宏：`HCI_MAX_ISO_SIZE`
- 大写宏：`HCI_RUNNING`
- 大写宏：`ISO`
- 大写宏：`IS_ERR`
- 大写宏：`MTK_ISO_IFNUM`
- 大写宏：`NULL`
- 大写宏：`PTR_ERR`
- 大写宏：`URB_FREE_BUFFER`
- 外部类型：`Avoid`
- 外部类型：`Complete`
- 外部类型：`Failed`
- 外部类型：`It`
- 外部类型：`btmtk_data`
- 外部类型：`gfp_t`
- 外部类型：`hci_dev`
- 外部类型：`hci_iso_hdr`
- 外部类型：`sk_buff`
- 外部类型：`urb`
- 外部类型：`usb_interface`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-36060cc717` → 本草稿移入 `cases/defect/auto-linux-36060cc717/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
