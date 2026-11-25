# auto-linux-894f348717

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
| 外部依赖数（dep_count） | 21 |
| 编译错误数（gcc syntax-only） | 12（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #8d7fe9885c62fc9a95bf55d9db244d9672a8aadc (https://github.com/torvalds/linux/commit/8d7fe9885c62fc9a95bf55d9db244d9672a8aadc)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 595；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 8d7fe9885c62fc9a95bf55d9db244d9672a8aadc Merge tag 'hid-for-linus-2026112301' of git://git.kernel.org/pub/scm/linux/kernel/git/hid/hid :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -584,16 +584,24 @@ static int asus_raw_event(struct hid_device *hdev,
 
 static int asus_kbd_set_report(struct hid_device *hdev, const u8 *buf, size_t buf_size)
 {
+	int ret;
 	u8 *dmabuf __free(kfree) = kmemdup(buf, buf_size, GFP_KERNEL);
 	if (!dmabuf)
 		return -ENOMEM;
 
+	/* The device may be runtime suspended, e.g. for a deferred backlight change */
+	ret = hid_hw_power(hdev, PM_HINT_FULLON);
+	if (ret < 0)
+		return ret;
+
 	/*
 	 * The report ID should be set from the incoming buffer due to LED and key
 	 * interfaces having different pages
 	 */
-	return hid_hw_raw_request(hdev, buf[0], dmabuf, buf_size, HID_FEATURE_REPORT,
-				  HID_REQ_SET_REPORT);
+	ret = hid_hw_raw_request(hdev, buf[0], dmabuf, buf_size, HID_FEATURE_REPORT,
+				 HID_REQ_SET_REPORT);
+	hid_hw_power(hdev, PM_HINT_NORMAL);
+	return ret;
 }
 
 static int asus_kbd_init(struct hid_device *hdev, u8 report_id)
@@ -1363,6 +1371,8 @@ static int asus_input_mapping(struct hid_device *hdev,
 		case 0xe2: /* Mute */
 		case 0xe9: /* Volume up */
 		case 0xea: /* Volume down */
+		case 0x6f: /* Display brightness increment */
+		case 0x70: /* Display brightness decrement */
 			return 0;
 		default:
 			/* Ignore dummy Consumer usages which make the
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__free`
- 外部函数：`hid_err`
- 外部函数：`hid_hw_raw_request`
- 外部函数：`hid_warn`
- 外部函数：`kmemdup`
- 外部函数：`kzalloc`
- 外部函数：`memcmp`
- 大写宏：`ENOMEM`
- 大写宏：`GFP_KERNEL`
- 大写宏：`HID_FEATURE_REPORT`
- 大写宏：`HID_REQ_GET_REPORT`
- 大写宏：`HID_REQ_SET_REPORT`
- 大写宏：`LED`
- 外部类型：`Asus`
- 外部类型：`Consumer`
- 外部类型：`Do`
- 外部类型：`Ignore`
- 外部类型：`Mute`
- 外部类型：`The`
- 外部类型：`They`
- 外部类型：`Volume`
- 外部类型：`hid_device`
- 外部类型：`size_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-894f348717` → 本草稿移入 `cases/defect/auto-linux-894f348717/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
