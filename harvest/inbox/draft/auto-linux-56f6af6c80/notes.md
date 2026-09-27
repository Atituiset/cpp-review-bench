# auto-linux-56f6af6c80

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#fd179f8a05be3ccae366b9b96e176b51fbe54aab](https://github.com/torvalds/linux/commit/fd179f8a05be3ccae366b9b96e176b51fbe54aab) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-09-27 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 16 |
| 编译错误数（gcc syntax-only） | 8（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #fd179f8a05be3ccae366b9b96e176b51fbe54aab (https://github.com/torvalds/linux/commit/fd179f8a05be3ccae366b9b96e176b51fbe54aab)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 79；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT fd179f8a05be3ccae366b9b96e176b51fbe54aab Merge tag 'ata-7.3-rc5' of git://git.kernel.org/pub/scm/linux/kernel/git/libata/linux :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -76,7 +76,7 @@ enum ata_quirks {
 	__ATA_QUIRK_MAX_SEC,		/* Limit max sectors */
 	__ATA_QUIRK_MAX_TRIM_128M,	/* Limit max trim size to 128M */
 	__ATA_QUIRK_NO_NCQ_ON_ATI,	/* Disable NCQ on ATI chipset */
-	__ATA_QUIRK_NO_LPM_ON_ATI,	/* Disable LPM on ATI chipset */
+	__ATA_QUIRK_NO_LPM_ON_ATI_AND_AMD, /* Disable LPM on ATI and AMD chipsets */
 	__ATA_QUIRK_NO_ID_DEV_LOG,	/* Identify device log missing */
 	__ATA_QUIRK_NO_LOG_DIR,		/* Do not read log directory */
 	__ATA_QUIRK_NO_FUA,		/* Do not use FUA */
@@ -115,7 +115,7 @@ enum {
 	ATA_QUIRK_MAX_SEC		= BIT_ULL(__ATA_QUIRK_MAX_SEC),
 	ATA_QUIRK_MAX_TRIM_128M		= BIT_ULL(__ATA_QUIRK_MAX_TRIM_128M),
 	ATA_QUIRK_NO_NCQ_ON_ATI		= BIT_ULL(__ATA_QUIRK_NO_NCQ_ON_ATI),
-	ATA_QUIRK_NO_LPM_ON_ATI		= BIT_ULL(__ATA_QUIRK_NO_LPM_ON_ATI),
+	ATA_QUIRK_NO_LPM_ON_ATI_AND_AMD	= BIT_ULL(__ATA_QUIRK_NO_LPM_ON_ATI_AND_AMD),
 	ATA_QUIRK_NO_ID_DEV_LOG		= BIT_ULL(__ATA_QUIRK_NO_ID_DEV_LOG),
 	ATA_QUIRK_NO_LOG_DIR		= BIT_ULL(__ATA_QUIRK_NO_LOG_DIR),
 	ATA_QUIRK_NO_FUA		= BIT_ULL(__ATA_QUIRK_NO_FUA),
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 大写宏：`ATA_QUIRK_MAX_SEC`
- 大写宏：`ATA_QUIRK_MAX_TRIM_128M`
- 大写宏：`ATA_QUIRK_NO_FUA`
- 大写宏：`ATA_QUIRK_NO_ID_DEV_LOG`
- 大写宏：`ATA_QUIRK_NO_LOG_DIR`
- 大写宏：`ATA_QUIRK_NO_LPM_ON_ATI`
- 大写宏：`ATA_QUIRK_NO_NCQ_ON_ATI`
- 大写宏：`ATI`
- 大写宏：`BIT_ULL`
- 大写宏：`FUA`
- 大写宏：`LPM`
- 大写宏：`NCQ`
- 外部类型：`Disable`
- 外部类型：`Do`
- 外部类型：`Identify`
- 外部类型：`Limit`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-56f6af6c80` → 本草稿移入 `cases/defect/auto-linux-56f6af6c80/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
