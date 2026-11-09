# auto-linux-58414cb2eb

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#25fccf436e6e978954bb2774170d1094889c3fc7](https://github.com/torvalds/linux/commit/25fccf436e6e978954bb2774170d1094889c3fc7) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-09 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 70 |
| 编译错误数（gcc syntax-only） | 81（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #25fccf436e6e978954bb2774170d1094889c3fc7 (https://github.com/torvalds/linux/commit/25fccf436e6e978954bb2774170d1094889c3fc7)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 1597；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 25fccf436e6e978954bb2774170d1094889c3fc7 Merge tag 'firewire-fixes-7.4-rc2' of git://git.kernel.org/pub/scm/linux/kernel/git/ieee1394/linux1394 :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1593,12 +1593,13 @@ static void handle_at_local_packets(struct at_local *local, struct fw_ohci *ohci
 			void (*handle_at_local_packet)(struct fw_ohci *, struct fw_packet *))
 {
 	struct fw_packet *packet;
+	unsigned long flags;
 
-	spin_lock(&local->lock);
+	spin_lock_irqsave(&local->lock, flags);
 
 	while ((packet = list_first_entry_or_null(&local->list, typeof(*packet), link_for_local))) {
 		list_del_init(&packet->link_for_local);
-		spin_unlock(&local->lock);
+		spin_unlock_irqrestore(&local->lock, flags);
 
 		if (unlikely(packet->ack != 0)) {
 			// This case is active when the call of at_context_queue_packet() returns
@@ -1608,10 +1609,10 @@ static void handle_at_local_packets(struct at_local *local, struct fw_ohci *ohci
 			handle_at_local_packet(ohci, packet);
 		}
 
-		spin_lock(&local->lock);
+		spin_lock_irqsave(&local->lock, flags);
 	}
 
-	spin_unlock(&local->lock);
+	spin_unlock_irqrestore(&local->lock, flags);
 }
 
 static void at_request_local_work(struct work_struct *work)
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__aligned`
- 外部函数：`async_header_get_data_length`
- 外部函数：`async_header_get_extended_tcode`
- 外部函数：`async_header_get_offset`
- 外部函数：`async_header_get_tcode`
- 外部函数：`at_context_queue_packet`
- 外部函数：`at_context_transmit`
- 外部函数：`be32_to_cpu`
- 外部函数：`callback`
- 外部函数：`container_of`
- 外部函数：`cpu_to_be32`
- 外部函数：`cycle_time_to_ohci_tstamp`
- 外部函数：`dev_err`
- 外部函数：`done`
- 外部函数：`from_work`
- 外部函数：`fw_core_handle_request`
- 外部函数：`fw_core_handle_response`
- 外部函数：`fw_fill_response`
- 外部函数：`handle_at_local_packet`
- 外部函数：`in_range`
- 外部函数：`list_del_init`
- 外部函数：`list_first_entry_or_null`
- 外部函数：`memset`
- 外部函数：`ohci_err`
- 外部函数：`readl`
- 外部函数：`reg_write`
- 外部函数：`spin_lock`
- 外部函数：`spin_unlock`
- 外部函数：`tcode_is_block_packet`
- 外部函数：`tcode_is_read_request`
- 外部函数：`typeof`
- 外部函数：`unlikely`
- 外部函数：`void`
- 外部函数：`writel`
- 大写宏：`ACK_PENDING`
- 大写宏：`CSR`
- 大写宏：`CSR_BUS_MANAGER_ID`
- 大写宏：`CSR_CHANNELS_AVAILABLE_LO`
- 大写宏：`CSR_CONFIG_ROM`
- 大写宏：`CSR_CONFIG_ROM_END`
- 大写宏：`CSR_MAINT_CONTROL`
- 大写宏：`CSR_REGISTER_BASE`
- 大写宏：`DIV_ROUND_UP`
- 大写宏：`EXTCODE_COMPARE_SWAP`
- 大写宏：`MAX`
- 大写宏：`NULL`
- 大写宏：`PAGE_SIZE`
- 大写宏：`RCODE_ADDRESS_ERROR`
- 大写宏：`RCODE_BUSY`
- 大写宏：`RCODE_COMPLETE`
- 大写宏：`RCODE_TYPE_ERROR`
- 大写宏：`TCODE_LOCK_REQUEST`
- 大写宏：`TCODE_READ_QUADLET_REQUEST`
- 外部类型：`Emulate`
- 外部类型：`Finish`
- 外部类型：`Never`
- 外部类型：`OHCI1394_CSRCompareData`
- 外部类型：`OHCI1394_CSRControl`
- 外部类型：`OHCI1394_CSRData`
- 外部类型：`OHCI1394_IsochronousCycleTimer`
- 外部类型：`Spinlock`
- 外部类型：`This`
- 外部类型：`Timestamping`
- 外部类型：`context`
- 外部类型：`dma_addr_t`
- 外部类型：`fw_card`
- 外部类型：`fw_iso_context`
- 外部类型：`fw_packet`
- 外部类型：`list_head`
- 外部类型：`mutex`
- 外部类型：`page`
- 外部类型：`size_t`
- 外部类型：`spinlock_t`
- 外部类型：`work_struct`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-58414cb2eb` → 本草稿移入 `cases/defect/auto-linux-58414cb2eb/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
