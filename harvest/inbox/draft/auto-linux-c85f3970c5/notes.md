# auto-linux-c85f3970c5

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#6a66840a68683d9361a9b31faf3bd8104428ad29](https://github.com/torvalds/linux/commit/6a66840a68683d9361a9b31faf3bd8104428ad29) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-16 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 313 |
| 编译错误数（gcc syntax-only） | 350（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #6a66840a68683d9361a9b31faf3bd8104428ad29 (https://github.com/torvalds/linux/commit/6a66840a68683d9361a9b31faf3bd8104428ad29)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 2627；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 6a66840a68683d9361a9b31faf3bd8104428ad29 Merge tag 'hyperv-fixes-signed-20261015' of git://git.kernel.org/pub/scm/linux/kernel/git/hyperv/linux :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1040,19 +1040,38 @@ static void put_pcichild(struct hv_pci_dev *hpdev)
 
 /*
  * There is no good way to get notified from vmbus_onoffer_rescind(),
- * so let's use polling here, since this is not a hot path.
+ * so let's use polling here, since this is not a hot path. If
+ * wait_for_response() has been polling for 2 minutes
+ * without either a rescind or completion, add a warning.
  */
+#define PCI_RESPONSE_WARN_POLL_COUNT 1200
+
 static int wait_for_response(struct hv_device *hdev,
-			     struct completion *comp)
+			     struct completion *comp,
+			     const char *msg_type)
 {
+	u64 counter = 0;
+
 	while (true) {
 		if (hdev->channel->rescind) {
 			dev_warn_once(&hdev->device, "The device is gone.\n");
 			return -ENODEV;
 		}
 
-		if (wait_for_completion_timeout(comp, HZ / 10))
+		counter++;
+
+		if (wait_for_completion_timeout(comp, HZ / 10)) {
+			if (counter > PCI_RESPONSE_WARN_POLL_COUNT)
+				dev_warn(&hdev->device,
+					 "Late %s completion arrived.\n", msg_type);
 			break;
+		}
+
+		if (counter == PCI_RESPONSE_WARN_POLL_COUNT) {
+			dev_err(&hdev->device,
+				"%s stuck waiting for response, relid = %u\n",
+				msg_type, hdev->channel->offermsg.child_relid);
+		}
 	}
 
 	return 0;
@@ -1518,7 +1537,8 @@ static int hv_read_config_block(struct pci_dev *pdev, void *buf,
 	if (ret)
 		return ret;
 
-	ret = wait_for_response(hbus->hdev, &comp_pkt.comp_pkt.host_event);
+	ret = wait_for_response(hbus->hdev, &comp_pkt.comp_pkt.host_event,
+				"PCI_READ_BLOCK");
 	if (ret)
 		return ret;
 
@@ -1607,7 +1627,8 @@ static int hv_write_config_block(struct pci_dev *pdev, void *buf,
 	if (ret)
 		return ret;
 
-	ret = wait_for_response(hbus->hdev, &comp_pkt.host_event);
+	ret = wait_for_response(hbus->hdev, &comp_pkt.host_event,
+				"PCI_WRITE_BLOCK");
 	if (ret)
 		return ret;
 
@@ -2624,7 +2645,8 @@ static struct hv_pci_dev *new_pcichild_device(struct hv_pcibus_device *hbus,
 	if (ret)
 		goto error;
 
-	if (wait_for_response(hbus->hdev, &comp_pkt.host_event))
+	if (wait_for_response(hbus->hdev, &comp_pkt.host_event,
+			      "PCI_QUERY_RESOURCE_REQUIREMENTS"))
 		goto error;
 
 	hpdev->desc = *desc;
@@ -3256,7 +3278,8 @@ static int hv_pci_protocol_negotiation(struct hv_device *hdev,
 				(unsigned long)pkt, VM_PKT_DATA_INBAND,
 				VMBUS_DATA_PACKET_FLAG_COMPLETION_REQUESTED);
 		if (!ret)
-			ret = wait_for_response(hdev, &comp_pkt.host_event);
+			ret = wait_for_response(hdev, &comp_pkt.host_event,
+						"PCI_QUERY_PROTOCOL_VERSION");
 
 		if (ret) {
 			dev_err(&hdev->device,
@@ -3476,7 +3499,8 @@ static int hv_pci_enter_d0(struct hv_device *hdev)
 			       (unsigned long)pkt, VM_PKT_DATA_INBAND,
 			       VMBUS_DATA_PACKET_FLAG_COMPLETION_REQUESTED);
 	if (!ret)
-		ret = wait_for_response(hdev, &comp_pkt.host_event);
+		ret = wait_for_response(hdev, &comp_pkt.host_event,
+					"PCI_BUS_D0ENTRY");
 
 	if (ret)
 		goto exit;
@@ -3553,7 +3577,8 @@ static int hv_pci_query_relations(struct hv_device *hdev)
 	ret = vmbus_sendpacket(hdev->channel, &message, sizeof(message),
 			       0, VM_PKT_DATA_INBAND, 0);
 	if (!ret)
-		ret = wait_for_response(hdev, &comp);
+		ret = wait_for_response(hdev, &comp,
+					"PCI_QUERY_BUS_RELATIONS");
 
 	/*
 	 * In the case of fast device addition/removal, it's possible that
@@ -3644,7 +3669,8 @@ static int hv_send_resources_allocated(struct hv_device *hdev)
 				VM_PKT_DATA_INBAND,
 				VMBUS_DATA_PACKET_FLAG_COMPLETION_REQUESTED);
 		if (!ret)
-			ret = wait_for_response(hdev, &comp_pkt.host_event);
+			ret = wait_for_response(hdev, &comp_pkt.host_event,
+						"PCI_RESOURCE_ASSIGNED");
 		if (ret)
 			break;
 
@@ -4155,6 +4181,9 @@ static struct hv_driver hv_pci_drv = {
 	.remove		= hv_pci_remove,
 	.suspend	= hv_pci_suspend,
 	.resume		= hv_pci_resume,
+	.driver = {
+		.probe_type = PROBE_PREFER_ASYNCHRONOUS,
+	},
 };
 
 static void __exit exit_hv_pci_drv(void)
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__builtin_clzll`
- 外部函数：`__counted_by`
- 外部函数：`__vmbus_request_addr_match`
- 外部函数：`and`
- 外部函数：`block_invalidate`
- 外部函数：`cmpxchg`
- 外部函数：`complete`
- 外部函数：`completion_func`
- 外部函数：`container_of`
- 外部函数：`cpumask_first_and`
- 外部函数：`cpumask_next_wrap`
- 外部函数：`defined`
- 外部函数：`destroy_workqueue`
- 外部函数：`dev_err`
- 外部函数：`dev_err_once`
- 外部函数：`dev_info`
- 外部函数：`dev_warn`
- 外部函数：`dev_warn_once`
- 外部函数：`dpm_suspend`
- 外部函数：`flush_workqueue`
- 外部函数：`guard`
- 外部函数：`hibernation_platform_enter`
- 外部函数：`hv_cpu_number_to_vp_number`
- 外部函数：`hv_do_hypercall`
- 外部函数：`hv_get_drvdata`
- 外部函数：`hv_irq_unmask`
- 外部函数：`hv_pci_assign_numa_node`
- 外部函数：`hv_pci_assign_slots`
- 外部函数：`hv_result_success`
- 外部函数：`hv_send_resource_released`
- 外部函数：`init_completion`
- 外部函数：`iounmap`
- 外部函数：`irq_data_get_effective_affinity_mask`
- 外部函数：`irq_data_get_msi_desc`
- 外部函数：`irq_domain_free_fwnode`
- 外部函数：`irq_domain_remove`
- 外部函数：`irq_get_irq_data`
- 外部函数：`irq_set_chip_data`
- 外部函数：`irqd_cfg`
- 外部函数：`kfree`
- 外部函数：`kmalloc`
- 外部函数：`kzalloc`
- 外部函数：`kzalloc_flex`
- 外部函数：`kzalloc_obj`
- 外部函数：`list_add_tail`
- 外部函数：`list_del`
- 外部函数：`list_empty`
- 外部函数：`list_first_entry`
- 外部函数：`list_move_tail`
- 外部函数：`lock_requestor`
- 外部函数：`mb`
- 外部函数：`memcpy`
- 外部函数：`memset`
- 外部函数：`msi_desc_to_pci_dev`
- 外部函数：`mutex_lock`
- 外部函数：`mutex_unlock`
- 外部函数：`new_pcichild_device`
- 外部函数：`packet`
- 外部函数：`pci_destroy_slot`
- 外部函数：`pci_dev_put`
- 外部函数：`pci_domain_nr`
- 外部函数：`pci_get_domain_bus_and_slot`
- 外部函数：`pci_lock_rescan_remove`
- 外部函数：`pci_remove_root_bus`
- 外部函数：`pci_scan_child_bus`
- 外部函数：`pci_stop_and_remove_bus_device`
- 外部函数：`pci_stop_root_bus`
- 外部函数：`pci_unlock_rescan_remove`
- 外部函数：`pci_walk_bus`
- 外部函数：`queue_work`
- 外部函数：`raw_spin_lock_irqsave`
- 外部函数：`readb`
- 外部函数：`readl`
- 外部函数：`readw`
- 外部函数：`refcount_dec_and_test`
- 外部函数：`refcount_inc`
- 外部函数：`register`
- 外部函数：`reinit_completion`
- 外部函数：`resource_size`
- 外部函数：`round_up`
- 外部函数：`spin_lock_irqsave`
- 外部函数：`spin_unlock_irqrestore`
- 外部函数：`struct_size`
- 外部函数：`survey_child_resources`
- 外部函数：`tasklet_disable`
- 外部函数：`tasklet_disable_in_atomic`
- 外部函数：`tasklet_enable`
- 外部函数：`this_cpu_ptr`
- 外部函数：`try_wait_for_completion`
- 外部函数：`udelay`
- 外部函数：`unlikely`
- 外部函数：`unlock_requestor`
- 外部函数：`unmask`
- 外部函数：`vmbus_close`
- 外部函数：`vmbus_driver_unregister`
- 外部函数：`vmbus_free_mmio`
- 外部函数：`vmbus_open`
- 外部函数：`vmbus_recvpacket_raw`
- 外部函数：`vmbus_request_addr_match`
- 外部函数：`vmbus_rescind_cleanup`
- 外部函数：`vmbus_reset_channel_cb`
- 外部函数：`vmbus_sendpacket`
- 外部函数：`vmbus_sendpacket_getid`
- 外部函数：`void`
- 外部函数：`wait_for_completion_timeout`
- 外部函数：`wait_for_response`
- 外部函数：`wmb`
- 外部函数：`writeb`
- 外部函数：`writel`
- 外部函数：`writew`
- 大写宏：`BAR`
- 大写宏：`BUS_RELATIONS`
- 大写宏：`CONFIG_ARM64`
- 大写宏：`CONFIG_X86`
- 大写宏：`CPU`
- 大写宏：`CPU_AFFINITY_ALL`
- 大写宏：`DEFINE_SPINLOCK`
- 大写宏：`DELIVERY_MODE`
- 大写宏：`EINVAL`
- 大写宏：`EJECT`
- 大写宏：`ENOBUFS`
- 大写宏：`ENODEV`
- 大写宏：`ENOENT`
- 大写宏：`ENOMEM`
- 大写宏：`ENOTEMPTY`
- 大写宏：`EPROTO`
- 大写宏：`ETIMEDOUT`
- 大写宏：`GFP_ATOMIC`
- 大写宏：`GFP_KERNEL`
- 大写宏：`GFP_NOWAIT`
- 大写宏：`HVCALL_MMIO_READ`
- 大写宏：`HVCALL_MMIO_WRITE`
- 大写宏：`INIT_LIST_HEAD`
- 大写宏：`INIT_WORK`
- 大写宏：`IORESOURCE_BUSY`
- 大写宏：`IRQ`
- 大写宏：`MMIO`
- 大写宏：`MSI`
- 大写宏：`MSI_DESC_ASSOCIATED`
- 大写宏：`NULL`
- 大写宏：`PAGE_SIZE`
- 大写宏：`PCI`
- 大写宏：`PCI_BASE_ADDRESS_0`
- 大写宏：`PCI_BASE_ADDRESS_MEM_MASK`
- 大写宏：`PCI_BASE_ADDRESS_MEM_TYPE_64`
- 大写宏：`PCI_BUS_D0ENTRY`
- 大写宏：`PCI_BUS_D0EXIT`
- 大写宏：`PCI_BUS_RELATIONS`
- 大写宏：`PCI_BUS_RELATIONS2`
- 大写宏：`PCI_CACHE_LINE_SIZE`
- 大写宏：`PCI_CAPABILITY_LIST`
- 大写宏：`PCI_CLASS_REVISION`
- 大写宏：`PCI_COMMAND`
- 大写宏：`PCI_COMMAND_MEMORY`
- 大写宏：`PCI_CREATE_INTERRUPT_MESSAGE`
- 大写宏：`PCI_CREATE_INTERRUPT_MESSAGE2`
- 大写宏：`PCI_CREATE_INTERRUPT_MESSAGE3`
- 大写宏：`PCI_DELETE_INTERRUPT_MESSAGE`
- 大写宏：`PCI_DEVFN`
- 大写宏：`PCI_EJECT`
- 大写宏：`PCI_EJECTION_COMPLETE`
- 大写宏：`PCI_ERROR_RESPONSE`
- 大写宏：`PCI_FUNC`
- 大写宏：`PCI_INTERRUPT_LINE`
- 大写宏：`PCI_INTERRUPT_PIN`
- 大写宏：`PCI_INVALIDATE_BLOCK`
- 大写宏：`PCI_MIN_GNT`
- 大写宏：`PCI_PROTOCOL_VERSION_1_1`
- 大写宏：`PCI_PROTOCOL_VERSION_1_2`
- 大写宏：`PCI_PROTOCOL_VERSION_1_3`
- 大写宏：`PCI_PROTOCOL_VERSION_1_4`
- 大写宏：`PCI_QUERY_BUS_RELATIONS`
- 大写宏：`PCI_QUERY_PROTOCOL_VERSION`
- 大写宏：`PCI_RESOURCES_ASSIGNED`
- 大写宏：`PCI_RESOURCES_ASSIGNED2`
- 大写宏：`PCI_RESOURCES_RELEASED`
- 大写宏：`PCI_ROM_ADDRESS`
- 大写宏：`PCI_SLOT`
- 大写宏：`PCI_STD_NUM_BARS`
- 大写宏：`PCI_SUBSYSTEM_VENDOR_ID`
- 大写宏：`PCI_VENDOR_ID`
- 大写宏：`PIN`
- 大写宏：`RESCIND_CHANNEL`
- 大写宏：`ROM`
- 大写宏：`RS1`
- 大写宏：`VENDOR`
- 大写宏：`VMBUS_DATA_PACKET_FLAG_COMPLETION_REQUESTED`
- 大写宏：`VMBUS_RQST_ADDR_ANY`
- 大写宏：`VMBUS_RQST_ERROR`
- 大写宏：`VM_PKT_COMP`
- 大写宏：`VM_PKT_DATA_INBAND`
- 大写宏：`VSP`
- 大写宏：`WARN_ON_ONCE`
- 大写宏：`WS2022`
- 外部类型：`Actually`
- 外部类型：`After`
- 外部类型：`All`
- 外部类型：`As`
- 外部类型：`Ask`
- 外部类型：`At`
- 外部类型：`Attempt`
- 外部类型：`Avoid`
- 外部类型：`BARs`
- 外部类型：`Call`
- 外部类型：`Can`
- 外部类型：`Cast`
- 外部类型：`Cf`
- 外部类型：`Change`
- 外部类型：`Choose`
- 外部类型：`Clear`
- 外部类型：`Couldn`
- 外部类型：`Create`
- 外部类型：`Delete`
- 外部类型：`Do`
- 外部类型：`Ejection`
- 外部类型：`Entry`
- 外部类型：`First`
- 外部类型：`Flush`
- 外部类型：`For`
- 外部类型：`Free`
- 外部类型：`Handle`
- 外部类型：`Highest`
- 外部类型：`However`
- 外部类型：`Hyper`
- 外部类型：`IDs`
- 外部类型：`If`
- 外部类型：`In`
- 外部类型：`Initiate`
- 外部类型：`Interrupt`
- 外部类型：`Invalid`
- 外部类型：`Kdump`
- 外部类型：`Line`
- 外部类型：`List`
- 外部类型：`Make`
- 外部类型：`Move`
- 外部类型：`Multi`
- 外部类型：`Must`
- 外部类型：`Next`
- 外部类型：`No`
- 外部类型：`Note`
- 外部类型：`Only`
- 外部类型：`Otherwise`
- 外部类型：`Pass`
- 外部类型：`Pick`
- 外部类型：`Prevents`
- 外部类型：`Protect`
- 外部类型：`Protocol`
- 外部类型：`Pull`
- 外部类型：`Read`
- 外部类型：`Record`
- 外部类型：`Remove`
- 外部类型：`Request`
- 外部类型：`Retrying`
- 外部类型：`Reuse`
- 外部类型：`SSIDs`
- 外部类型：`See`
- 外部类型：`Sending`
- 外部类型：`Set`
- 外部类型：`Since`
- 外部类型：`Tell`
- 外部类型：`Test`
- 外部类型：`The`
- 外部类型：`There`
- 外部类型：`This`
- 外部类型：`Throw`
- 外部类型：`To`
- 外部类型：`Truncates`
- 外部类型：`Unexpected`
- 外部类型：`Unimplemented`
- 外部类型：`Use`
- 外部类型：`Using`
- 外部类型：`VMBus`
- 外部类型：`VMbus`
- 外部类型：`Vibranium`
- 外部类型：`We`
- 外部类型：`What`
- 外部类型：`When`
- 外部类型：`Win10`
- 外部类型：`Write`
- 外部类型：`Zero`
- 外部类型：`completion`
- 外部类型：`cpumask`
- 外部类型：`device`
- 外部类型：`fwnode_handle`
- 外部类型：`hv_device`
- 外部类型：`hv_mmio_read_input`
- 外部类型：`hv_mmio_read_output`
- 外部类型：`hv_mmio_write_input`
- 外部类型：`hv_msi_desc`
- 外部类型：`hv_msi_desc2`
- 外部类型：`hv_msi_desc3`
- 外部类型：`irq_cfg`
- 外部类型：`irq_data`
- 外部类型：`irq_domain`
- 外部类型：`list_head`
- 外部类型：`msi_desc`
- 外部类型：`msi_msg`
- 外部类型：`mutex`
- 外部类型：`pci_bus`
- 外部类型：`pci_config_window`
- 外部类型：`pci_create_int_response`
- 外部类型：`pci_delete_interrupt`
- 外部类型：`pci_dev`
- 外部类型：`pci_host_bridge`
- 外部类型：`pci_protocol_version_t`
- 外部类型：`pci_response`
- 外部类型：`pci_slot`
- 外部类型：`pci_sysdata`
- 外部类型：`pci_version_request`
- 外部类型：`phys_addr_t`
- 外部类型：`refcount_t`
- 外部类型：`resource`
- 外部类型：`resource_size_t`
- 外部类型：`size_t`
- 外部类型：`spinlock_t`
- 外部类型：`vmbus_channel`
- 外部类型：`vmpacket_descriptor`
- 外部类型：`work_struct`
- 外部类型：`workqueue_struct`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-c85f3970c5` → 本草稿移入 `cases/defect/auto-linux-c85f3970c5/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
