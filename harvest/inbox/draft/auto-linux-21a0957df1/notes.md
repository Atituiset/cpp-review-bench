# auto-linux-21a0957df1

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
| 外部依赖数（dep_count） | 23 |
| 编译错误数（gcc syntax-only） | 14（0=切片已达编译地板） |

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
@@ -66,6 +66,7 @@ struct snd_ump_endpoint {
 
 #if IS_ENABLED(CONFIG_SND_SEQUENCER)
 	struct snd_seq_device *seq_dev;
+	/* Published after seq_client; calls are covered by UMP core SRCU. */
 	const struct snd_seq_ump_ops *seq_ops;
 	void *seq_client;
 #endif
@@ -126,6 +127,10 @@ int snd_ump_receive_ump_val(struct snd_ump_endpoint *ump, u32 val);
 int snd_ump_switch_protocol(struct snd_ump_endpoint *ump, unsigned int protocol);
 void snd_ump_update_group_attrs(struct snd_ump_endpoint *ump);
 
+#if IS_ENABLED(CONFIG_SND_SEQUENCER)
+void snd_ump_detach_seq(struct snd_ump_endpoint *ump);
+#endif
+
 /*
  * Some definitions for UMP
  */
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`group`
- 外部函数：`index`
- 外部函数：`int`
- 外部函数：`snd_ump_switch_protocol`
- 外部函数：`snd_ump_update_group_attrs`
- 外部函数：`void`
- 大写宏：`CONFIG_SND_SEQUENCER`
- 大写宏：`CONFIG_SND_UMP_LEGACY_RAWMIDI`
- 大写宏：`IS_ENABLED`
- 大写宏：`MIDI1`
- 大写宏：`SNDRV_UMP_MAX_GROUPS`
- 大写宏：`UMP`
- 外部类型：`Some`
- 外部类型：`Stream`
- 外部类型：`list_head`
- 外部类型：`mutex`
- 外部类型：`snd_rawmidi`
- 外部类型：`snd_rawmidi_file`
- 外部类型：`snd_rawmidi_substream`
- 外部类型：`snd_seq_device`
- 外部类型：`snd_ump_block_info`
- 外部类型：`snd_ump_endpoint_info`
- 外部类型：`spinlock_t`
- 外部类型：`ump_cvt_to_ump`
- 外部类型：`wait_queue_head_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-21a0957df1` → 本草稿移入 `cases/defect/auto-linux-21a0957df1/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
