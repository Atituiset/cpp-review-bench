# auto-linux-5eed9068d2

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#74f6055d0c5997b13e6a5253fdc2263f3af90408](https://github.com/torvalds/linux/commit/74f6055d0c5997b13e6a5253fdc2263f3af90408) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-01 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 98 |
| 编译错误数（gcc syntax-only） | 244（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #74f6055d0c5997b13e6a5253fdc2263f3af90408 (https://github.com/torvalds/linux/commit/74f6055d0c5997b13e6a5253fdc2263f3af90408)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 289；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 74f6055d0c5997b13e6a5253fdc2263f3af90408 Merge tag 'asoc-fix-v7.4-merge-window' of https://git.kernel.org/pub/scm/linux/kernel/git/broonie/sound into for-linus :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -178,6 +178,16 @@ static int imx_rpmsg_pcm_hw_params(struct snd_soc_component *component,
 	return 0;
 }
 
+static int imx_rpmsg_pcm_hw_free(struct snd_soc_component *component,
+				 struct snd_pcm_substream *substream)
+{
+	struct rpmsg_info *info = dev_get_drvdata(component->dev);
+
+	flush_workqueue(info->rpmsg_wq);
+
+	return 0;
+}
+
 static snd_pcm_uframes_t imx_rpmsg_pcm_pointer(struct snd_soc_component *component,
 					       struct snd_pcm_substream *substream)
 {
@@ -285,9 +295,6 @@ static int imx_rpmsg_pcm_close(struct snd_soc_component *component,
 	struct rpmsg_info *info = dev_get_drvdata(component->dev);
 	struct rpmsg_msg *msg;
 
-	/* Flush work in workqueue to make TX_CLOSE is the last message */
-	flush_workqueue(info->rpmsg_wq);
-
 	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
 		msg = &info->msg[TX_CLOSE];
 		msg->s_msg.header.cmd = TX_CLOSE;
@@ -624,6 +631,7 @@ static const struct snd_soc_component_driver imx_rpmsg_soc_component = {
 	.open		= imx_rpmsg_pcm_open,
 	.close		= imx_rpmsg_pcm_close,
 	.hw_params	= imx_rpmsg_pcm_hw_params,
+	.hw_free	= imx_rpmsg_pcm_hw_free,
 	.trigger	= imx_rpmsg_pcm_trigger,
 	.pointer	= imx_rpmsg_pcm_pointer,
 	.ack		= imx_rpmsg_pcm_ack,
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`close`
- 外部函数：`dev_get_drvdata`
- 外部函数：`dev_warn`
- 外部函数：`flush_workqueue`
- 外部函数：`guard`
- 外部函数：`memcpy`
- 外部函数：`open`
- 外部函数：`params_channels`
- 外部函数：`params_format`
- 外部函数：`params_rate`
- 外部函数：`queue_work`
- 外部函数：`send_message`
- 外部函数：`snd_pcm_hw_constraint_integer`
- 外部函数：`snd_pcm_period_elapsed`
- 外部函数：`snd_soc_rtd_to_cpu`
- 外部函数：`snd_soc_set_runtime_hwparams`
- 外部函数：`snd_soc_substream_to_rtd`
- 外部函数：`timer_container_of`
- 外部函数：`timer_delete`
- 外部函数：`timer_setup`
- 大写宏：`EPIPE`
- 大写宏：`MSG_TYPE_A_NUM`
- 大写宏：`RPMSG_CH_LEFT`
- 大写宏：`RPMSG_CH_STEREO`
- 大写宏：`RPMSG_DSD_U16_LE`
- 大写宏：`RPMSG_DSD_U32_LE`
- 大写宏：`RPMSG_S16_LE`
- 大写宏：`RPMSG_S24_LE`
- 大写宏：`RPMSG_S32_LE`
- 大写宏：`RX_CLOSE`
- 大写宏：`RX_HW_PARAM`
- 大写宏：`RX_OPEN`
- 大写宏：`RX_PERIOD_DONE`
- 大写宏：`RX_POINTER`
- 大写宏：`SNDRV_PCM_FORMAT_DSD_U16_LE`
- 大写宏：`SNDRV_PCM_FORMAT_DSD_U32_LE`
- 大写宏：`SNDRV_PCM_FORMAT_S16_LE`
- 大写宏：`SNDRV_PCM_FORMAT_S24_LE`
- 大写宏：`SNDRV_PCM_HW_PARAM_PERIODS`
- 大写宏：`SNDRV_PCM_STREAM_PLAYBACK`
- 大写宏：`TX_CLOSE`
- 大写宏：`TX_HW_PARAM`
- 大写宏：`TX_OPEN`
- 大写宏：`TX_PERIOD_DONE`
- 大写宏：`TX_POINTER`
- 大写宏：`WORK_MAX_NUM`
- 外部类型：`ASoC`
- 外部类型：`Create`
- 外部类型：`Flush`
- 外部类型：`If`
- 外部类型：`Msg`
- 外部类型：`Queue`
- 外部类型：`Since`
- 外部类型：`fsl_rpmsg`
- 外部类型：`rpmsg_info`
- 外部类型：`rpmsg_msg`
- 外部类型：`rpmsg_s_msg`
- 外部类型：`snd_pcm_hardware`
- 外部类型：`snd_pcm_hw_params`
- 外部类型：`snd_pcm_substream`
- 外部类型：`snd_pcm_uframes_t`
- 外部类型：`snd_soc_component`
- 外部类型：`snd_soc_dai`
- 外部类型：`snd_soc_pcm_runtime`
- 外部类型：`stream_timer`
- 外部类型：`timer_list`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-5eed9068d2` → 本草稿移入 `cases/defect/auto-linux-5eed9068d2/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
