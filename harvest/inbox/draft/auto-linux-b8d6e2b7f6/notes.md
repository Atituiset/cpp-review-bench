# auto-linux-b8d6e2b7f6

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#2800868f63227c8237d70c41e70d441c919620d6](https://github.com/torvalds/linux/commit/2800868f63227c8237d70c41e70d441c919620d6) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-14 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 11 |
| 编译错误数（gcc syntax-only） | 9（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #2800868f63227c8237d70c41e70d441c919620d6 (https://github.com/torvalds/linux/commit/2800868f63227c8237d70c41e70d441c919620d6)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 1（原始 PR diff 行 574；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 2800868f63227c8237d70c41e70d441c919620d6 Merge tag 'sound-7.4-rc3' of git://git.kernel.org/pub/scm/linux/kernel/git/tiwai/sound :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -571,7 +571,6 @@ static const struct hda_model_fixup cs4208_models[] = {
 
 static const struct hda_quirk cs4208_fixup_tbl[] = {
 	SND_PCI_QUIRK_VENDOR(0x106b, "Apple", CS4208_MAC_AUTO),
-	SND_PCI_QUIRK(0x8086, 0x7270, "MacBookAir 7,2", CS4208_MAC_AUTO),
 	{} /* terminator */
 };
 
@@ -584,7 +583,6 @@ static const struct hda_quirk cs4208_mac_fixup_tbl[] = {
 	SND_PCI_QUIRK(0x106b, 0x7800, "MacPro 6,1", CS4208_MACMINI),
 	SND_PCI_QUIRK(0x106b, 0x7b00, "MacBookPro 12,1", CS4208_MBP11),
 	SND_PCI_QUIRK(0x106b, 0x7f00, "iMac 16,1", CS4208_MBP11),
-	SND_PCI_QUIRK(0x8086, 0x7270, "MacBookAir 7,2", CS4208_MBA6),
 	{} /* terminator */
 };
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 大写宏：`CS4208_MACMINI`
- 大写宏：`CS4208_MAC_AUTO`
- 大写宏：`CS4208_MBA6`
- 大写宏：`CS4208_MBP11`
- 大写宏：`SND_PCI_QUIRK`
- 大写宏：`SND_PCI_QUIRK_VENDOR`
- 外部类型：`Apple`
- 外部类型：`MacBookAir`
- 外部类型：`MacBookPro`
- 外部类型：`MacPro`
- 外部类型：`hda_quirk`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-b8d6e2b7f6` → 本草稿移入 `cases/defect/auto-linux-b8d6e2b7f6/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
