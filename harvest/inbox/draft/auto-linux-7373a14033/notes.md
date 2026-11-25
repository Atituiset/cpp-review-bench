# auto-linux-7373a14033

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#8d7fe9885c62fc9a95bf55d9db244d9672a8aadc](https://github.com/torvalds/linux/commit/8d7fe9885c62fc9a95bf55d9db244d9672a8aadc) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-25 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 43 |
| 编译错误数（gcc syntax-only） | 8（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #8d7fe9885c62fc9a95bf55d9db244d9672a8aadc (https://github.com/torvalds/linux/commit/8d7fe9885c62fc9a95bf55d9db244d9672a8aadc)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 8d7fe9885c62fc9a95bf55d9db244d9672a8aadc Merge tag 'hid-for-linus-2026112301' of git://git.kernel.org/pub/scm/linux/kernel/git/hid/hid :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -812,6 +812,22 @@ static int lenovo_raw_event(struct hid_device *hdev,
 		      size >= 4 && report->id == 0x03))
 		return lenovo_raw_event_TP_X12_tab(hdev, get_unaligned_le32(data));
 
+	/*
+	 * Lenovo ThinkPad T14s gen6/7 snapdragon version filter out spurious
+	 * F23 events when Fn key is pressed. F23 cannot be unmapped because of
+	 * the copilot key sending left-meta + left-shift + F23. Filter report 8
+	 * (keyboard) scancode 0x72 (F23) when the modifier byte is not 0x0a.
+	 */
+	if (unlikely(hdev->product == I2C_DEVICE_ID_ELAN_LENOVO_T14S_KEYBOARD &&
+		     size == 9 && data[0] == 8 && data[1] != 0x0a)) {
+		for (int i = 3; i < 9; i++)
+			if (data[i] == 0x72) {
+				memmove(&data[i], &data[i + 1], 8 - i);
+				data[8] = 0;
+				break;
+			}
+	}
+
 	return 0;
 }
 
@@ -1393,6 +1409,10 @@ static int lenovo_probe(struct hid_device *hdev,
 {
 	int ret;
 
+	/* Make the T14s  keyboard trackpoint a separate input-device */
+	if (hdev->product == I2C_DEVICE_ID_ELAN_LENOVO_T14S_KEYBOARD)
+		hdev->quirks |= HID_QUIRK_INPUT_PER_APP;
+
 	ret = hid_parse(hdev);
 	if (ret) {
 		hid_err(hdev, "hid_parse failed\n");
@@ -1558,6 +1578,7 @@ static const struct hid_device_id lenovo_devices[] = {
 	{ HID_USB_DEVICE(USB_VENDOR_ID_IBM, USB_DEVICE_ID_IBM_SCROLLPOINT_800DPI_OPTICAL_PRO) },
 	{ HID_USB_DEVICE(USB_VENDOR_ID_LENOVO, USB_DEVICE_ID_LENOVO_SCROLLPOINT_OPTICAL) },
 	{ HID_USB_DEVICE(USB_VENDOR_ID_LENOVO, USB_DEVICE_ID_LENOVO_TP10UBKBD) },
+	{ HID_I2C_DEVICE(USB_VENDOR_ID_ELAN, I2C_DEVICE_ID_ELAN_LENOVO_T14S_KEYBOARD) },
 	/*
 	 * Note bind to the HID_GROUP_GENERIC group, so that we only bind to the keyboard
 	 * part, while letting hid-multitouch.c handle the touchpad and trackpoint.
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`get_unaligned_le32`
- 外部函数：`hid_err`
- 外部函数：`hid_parse`
- 外部函数：`input_report_key`
- 外部函数：`input_sync`
- 大写宏：`F20`
- 大写宏：`HID_GROUP_GENERIC`
- 大写宏：`HID_USB_DEVICE`
- 大写宏：`KEY_BOOKMARKS`
- 大写宏：`KEY_F20`
- 大写宏：`KEY_KBDILLUMTOGGLE`
- 大写宏：`KEY_PERFORMANCE`
- 大写宏：`KEY_PICKUP_PHONE`
- 大写宏：`KEY_RFKILL`
- 大写宏：`KEY_SELECTIVE_SCREENSHOT`
- 大写宏：`LED`
- 大写宏：`MIC`
- 大写宏：`NULL`
- 大写宏：`PICKUP`
- 大写宏：`STAR`
- 大写宏：`TAB`
- 大写宏：`TAB1`
- 大写宏：`TAB2`
- 大写宏：`USB_DEVICE_ID_IBM_SCROLLPOINT_800DPI_OPTICAL_PRO`
- 大写宏：`USB_DEVICE_ID_LENOVO_SCROLLPOINT_OPTICAL`
- 大写宏：`USB_DEVICE_ID_LENOVO_TP10UBKBD`
- 大写宏：`USB_DEVICE_ID_LENOVO_X12_TAB`
- 大写宏：`USB_VENDOR_ID_IBM`
- 大写宏：`USB_VENDOR_ID_LENOVO`
- 大写宏：`X12`
- 外部类型：`Airplane`
- 外部类型：`Airplanemode`
- 外部类型：`BookMarks`
- 外部类型：`Fn`
- 外部类型：`Iterate`
- 外部类型：`Keyboard`
- 外部类型：`Note`
- 外部类型：`Phone`
- 外部类型：`Power`
- 外部类型：`Snipping`
- 外部类型：`Whereas`
- 外部类型：`hid_device`
- 外部类型：`hid_input`
- 外部类型：`input_dev`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-7373a14033` → 本草稿移入 `cases/defect/auto-linux-7373a14033/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
