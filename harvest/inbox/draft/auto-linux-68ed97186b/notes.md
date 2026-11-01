# auto-linux-68ed97186b 🟢 零依赖候选

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#e9954f3fbddd25e1a01b7e5a0b72fb8b564204e0](https://github.com/torvalds/linux/commit/e9954f3fbddd25e1a01b7e5a0b72fb8b564204e0) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-01 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 8 |
| 编译错误数（gcc syntax-only） | 0（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #e9954f3fbddd25e1a01b7e5a0b72fb8b564204e0 (https://github.com/torvalds/linux/commit/e9954f3fbddd25e1a01b7e5a0b72fb8b564204e0)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT e9954f3fbddd25e1a01b7e5a0b72fb8b564204e0 Merge tag 'sound-fix-7.4-rc1' of git://git.kernel.org/pub/scm/linux/kernel/git/tiwai/sound :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -209,6 +209,9 @@ struct snd_ctl_elem_value;
 
 #define CS35L56_GPIO_FN_GPIO				0x00000001
 
+/* DSP1_FW_VER */
+#define CS35L56_FW_MAIN_VERSION_MASK			GENMASK(19, 0)
+
 /* Mixer input sources */
 #define CS35L56_INPUT_SRC_NONE				0x00
 #define CS35L56_INPUT_SRC_ASP1RX1			0x08
@@ -446,6 +449,7 @@ int cs35l56_cal_set_status_get(struct cs35l56_base *cs35l56_base,
 int cs35l56_read_prot_status(struct cs35l56_base *cs35l56_base,
 			     bool *fw_missing, unsigned int *fw_version);
 void cs35l56_warn_if_firmware_missing(struct cs35l56_base *cs35l56_base);
+bool cs35l56_needs_wait_for_firmware_timer_expiry(struct cs35l56_base *cs35l56_base);
 void cs35l56_log_tuning(struct cs35l56_base *cs35l56_base, struct cs_dsp *cs_dsp);
 int cs35l56_hw_init(struct cs35l56_base *cs35l56_base);
 int cs35l56_get_speaker_id(struct cs35l56_base *cs35l56_base);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`cs35l56_get_speaker_id`
- 外部函数：`cs35l56_hw_init`
- 外部函数：`cs35l56_log_tuning`
- 外部函数：`cs35l56_read_prot_status`
- 外部函数：`cs35l56_warn_if_firmware_missing`
- 外部类型：`Mixer`
- 外部类型：`cs35l56_base`
- 外部类型：`cs_dsp`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-linux-68ed97186b` → 本草稿移入 `cases/defect/auto-linux-68ed97186b/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
