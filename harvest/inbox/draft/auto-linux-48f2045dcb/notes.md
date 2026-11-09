# auto-linux-48f2045dcb

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#e6f836ef7865fb454641febe080cf88a3c677186](https://github.com/torvalds/linux/commit/e6f836ef7865fb454641febe080cf88a3c677186) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-09 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 11 |
| 编译错误数（gcc syntax-only） | 23（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #e6f836ef7865fb454641febe080cf88a3c677186 (https://github.com/torvalds/linux/commit/e6f836ef7865fb454641febe080cf88a3c677186)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 11（原始 PR diff 行 4083；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT e6f836ef7865fb454641febe080cf88a3c677186 Merge tag 'ata-7.4-rc2' of git://git.kernel.org/pub/scm/linux/kernel/git/libata/linux :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -4080,8 +4080,20 @@ void ata_eh_finish(struct ata_port *ap)
 				ata_eh_qc_complete(qc);
 			}
 		} else {
-			if (qc->flags & ATA_QCFLAG_SENSE_VALID ||
-			    qc->flags & ATA_QCFLAG_EH_SUCCESS_CMD) {
+			if (qc->flags & ATA_QCFLAG_SENSE_VALID &&
+			    qc->flags & ATA_QCFLAG_RETRY) {
+				/*
+				 * The sense data asks for a retry, see
+				 * ata_eh_analyze_tf(). ata_eh_link_autopsy()
+				 * cleared qc->err_mask, so ata_eh_qc_retry()
+				 * would increment scmd->allowed. Leave
+				 * scmd->allowed alone, so that upper layer
+				 * only retries the command if it has not
+				 * already been retried too many times.
+				 */
+				__ata_eh_qc_complete(qc);
+			} else if (qc->flags & ATA_QCFLAG_SENSE_VALID ||
+				   qc->flags & ATA_QCFLAG_EH_SUCCESS_CMD) {
 				ata_eh_qc_complete(qc);
 			} else {
 				/* feed zero TF to sense generation */
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__ata_qc_complete`
- 外部函数：`ata_tag_valid`
- 外部函数：`scsi_eh_finish_cmd`
- 外部函数：`spin_lock_irqsave`
- 外部函数：`spin_unlock_irqrestore`
- 大写宏：`ATA_QCFLAG_EH_SUCCESS_CMD`
- 大写宏：`ATA_QCFLAG_SENSE_VALID`
- 大写宏：`WARN_ON`
- 外部类型：`ata_port`
- 外部类型：`ata_queued_cmd`
- 外部类型：`scsi_cmnd`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-48f2045dcb` → 本草稿移入 `cases/defect/auto-linux-48f2045dcb/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
