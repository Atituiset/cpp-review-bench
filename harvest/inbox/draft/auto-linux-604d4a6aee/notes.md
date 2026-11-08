# auto-linux-604d4a6aee

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#a41e364995bbcfa627418a70fe913c35b7510728](https://github.com/torvalds/linux/commit/a41e364995bbcfa627418a70fe913c35b7510728) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-08 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 93 |
| 编译错误数（gcc syntax-only） | 28（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #a41e364995bbcfa627418a70fe913c35b7510728 (https://github.com/torvalds/linux/commit/a41e364995bbcfa627418a70fe913c35b7510728)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 1282；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT a41e364995bbcfa627418a70fe913c35b7510728 Merge tag 'scsi-fixes' of git://git.kernel.org/pub/scm/linux/kernel/git/jejb/scsi :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1279,7 +1279,8 @@ static int blogic_sdev_configure(struct scsi_device *,
 				 struct queue_limits *lim);
 static void blogic_qcompleted_ccb(struct blogic_ccb *);
 static irqreturn_t blogic_inthandler(int, void *);
-static int blogic_resetadapter(struct blogic_adapter *, bool hard_reset);
+static int blogic_resetadapter(struct Scsi_Host *host, bool hard_reset)
+	__must_hold(&host->host_lock);
 static void blogic_msg(enum blogic_msglevel, char *, struct blogic_adapter *, ...);
 static int __init blogic_setup(char *);
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__attribute__`
- 外部函数：`blogic_inthandler`
- 外部函数：`blogic_msg`
- 外部函数：`blogic_qcompleted_ccb`
- 外部函数：`blogic_resetadapter`
- 外部函数：`blogic_setup`
- 外部函数：`void`
- 大写宏：`BLOGIC_ABRT_QUEUE`
- 大写宏：`BLOGIC_ADAPTER_SW_ERROR`
- 大写宏：`BLOGIC_AUTOREQSENSE_FAIL`
- 大写宏：`BLOGIC_BAD_CMD_PARAM`
- 大写宏：`BLOGIC_BAD_MSG_RCVD`
- 大写宏：`BLOGIC_BAD_RECONNECT`
- 大写宏：`BLOGIC_BDR`
- 大写宏：`BLOGIC_CCB_ACTIVE`
- 大写宏：`BLOGIC_CCB_COMPLETE`
- 大写宏：`BLOGIC_CCB_FREE`
- 大写宏：`BLOGIC_CCB_RESET`
- 大写宏：`BLOGIC_CHECKCONDITION`
- 大写宏：`BLOGIC_CMD_ABORT_BY_HOST`
- 大写宏：`BLOGIC_CMD_CMPLT_NORMAL`
- 大写宏：`BLOGIC_CMD_COMPLETE_ERROR`
- 大写宏：`BLOGIC_CMD_COMPLETE_GOOD`
- 大写宏：`BLOGIC_CMD_NOTFOUND`
- 大写宏：`BLOGIC_DATAIN_CHECKED`
- 大写宏：`BLOGIC_DATAOUT_CHECKED`
- 大写宏：`BLOGIC_DATA_OVERRUN`
- 大写宏：`BLOGIC_DATA_UNDERRUN`
- 大写宏：`BLOGIC_DEVBUSY`
- 大写宏：`BLOGIC_EISA_BUS`
- 大写宏：`BLOGIC_FLASHPOINT`
- 大写宏：`BLOGIC_HEADTAG`
- 大写宏：`BLOGIC_HW_BDR`
- 大写宏：`BLOGIC_HW_FAIL`
- 大写宏：`BLOGIC_HW_RESET`
- 大写宏：`BLOGIC_HW_TIMEOUT`
- 大写宏：`BLOGIC_INBOX_FREE`
- 大写宏：`BLOGIC_INITIATOR_CCB`
- 大写宏：`BLOGIC_INITIATOR_CCBB_RESIDUAL`
- 大写宏：`BLOGIC_INITIATOR_CCB_SG`
- 大写宏：`BLOGIC_INITIATOR_CCB_SG_RESIDUAL`
- 大写宏：`BLOGIC_INVALID_BUSPHASE`
- 大写宏：`BLOGIC_INVALID_CCB`
- 大写宏：`BLOGIC_INVALID_CMD_CODE`
- 大写宏：`BLOGIC_INVALID_OUTBOX_CODE`
- 大写宏：`BLOGIC_ISA_BUS`
- 大写宏：`BLOGIC_LINKCCB_BADLUN`
- 大写宏：`BLOGIC_LINK_CMD_CMPLT`
- 大写宏：`BLOGIC_LINK_CMD_CMPLT_FLAG`
- 大写宏：`BLOGIC_MBOX_ABORT`
- 大写宏：`BLOGIC_MBOX_START`
- 大写宏：`BLOGIC_MCA_BUS`
- 大写宏：`BLOGIC_MULTIMASTER`
- 大写宏：`BLOGIC_NOEXPECT_BUSFREE`
- 大写宏：`BLOGIC_NORESPONSE_TO_ATN`
- 大写宏：`BLOGIC_NOTX`
- 大写宏：`BLOGIC_OP_GOOD`
- 大写宏：`BLOGIC_ORDEREDTAG`
- 大写宏：`BLOGIC_OUTBOX_FREE`
- 大写宏：`BLOGIC_PARITY_ERR`
- 大写宏：`BLOGIC_PCI_BUS`
- 大写宏：`BLOGIC_RST_FROM_OTHERDEV`
- 大写宏：`BLOGIC_RSVDTAG`
- 大写宏：`BLOGIC_SELECT_TIMEOUT`
- 大写宏：`BLOGIC_SIMPLETAG`
- 大写宏：`BLOGIC_TAGQUEUE_REJECT`
- 大写宏：`BLOGIC_TGT_CCB`
- 大写宏：`BLOGIC_UNCHECKED_TX`
- 大写宏：`BLOGIC_UNKNOWN_BUS`
- 大写宏：`BLOGIC_VESA_BUS`
- 大写宏：`CONFIG_SCSI_FLASHPOINT`
- 大写宏：`SCCB`
- 外部类型：`Bit`
- 外部类型：`Bits`
- 外部类型：`BusLogic`
- 外部类型：`Byte`
- 外部类型：`Bytes`
- 外部类型：`Common`
- 外部类型：`Defined`
- 外部类型：`Driver`
- 外部类型：`Firmware`
- 外部类型：`FlashPoint`
- 外部类型：`Linux`
- 外部类型：`Manager`
- 外部类型：`MultiMaster`
- 外部类型：`Portion`
- 外部类型：`Scsi_Host`
- 外部类型：`blogic_msglevel`
- 外部类型：`dma_addr_t`
- 外部类型：`irqreturn_t`
- 外部类型：`list_head`
- 外部类型：`pci_dev`
- 外部类型：`queue_limits`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-604d4a6aee` → 本草稿移入 `cases/defect/auto-linux-604d4a6aee/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
