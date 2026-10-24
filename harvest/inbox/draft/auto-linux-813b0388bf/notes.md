# auto-linux-813b0388bf

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#4da9dedf3fc59137fd86d664693776aa38cb97f5](https://github.com/torvalds/linux/commit/4da9dedf3fc59137fd86d664693776aa38cb97f5) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-24 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 25 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #4da9dedf3fc59137fd86d664693776aa38cb97f5 (https://github.com/torvalds/linux/commit/4da9dedf3fc59137fd86d664693776aa38cb97f5)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 4da9dedf3fc59137fd86d664693776aa38cb97f5 Merge tag 'iommu-updates-v7.4' of git://git.kernel.org/pub/scm/linux/kernel/git/iommu/linux :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -21,6 +21,7 @@
 #include <linux/panic_notifier.h>
 #include <linux/ptrace.h>
 #include <linux/random.h>
+#include <linux/rhashtable.h>
 #include <linux/efi.h>
 #include <linux/kdebug.h>
 #include <linux/kmsg_dump.h>
@@ -78,6 +79,22 @@ static struct ctl_table_header *hv_ctl_table_hdr;
 u8 * __percpu *hv_synic_eventring_tail;
 EXPORT_SYMBOL_GPL(hv_synic_eventring_tail);
 
+struct hv_pci_busdata {
+	int pci_domain_nr;
+	u32 logical_dev_id_prefix;
+	struct rhash_head node;
+	struct rcu_head rcu;
+};
+
+static struct rhashtable hv_pci_bus_ht;
+static bool hv_pci_bus_ht_initialized;
+
+static const struct rhashtable_params hv_pci_bus_ht_params = {
+	.key_len	= sizeof_field(struct hv_pci_busdata, pci_domain_nr),
+	.key_offset	= offsetof(struct hv_pci_busdata, pci_domain_nr),
+	.head_offset	= offsetof(struct hv_pci_busdata, node),
+};
+
 /*
  * Hyper-V specific initialization and shutdown code that is
  * common across all architectures.  Called from architecture
@@ -86,6 +103,11 @@ EXPORT_SYMBOL_GPL(hv_synic_eventring_tail);
 
 void __init hv_common_free(void)
 {
+	if (hv_pci_bus_ht_initialized) {
+		rhashtable_destroy(&hv_pci_bus_ht);
+		hv_pci_bus_ht_initialized = false;
+	}
+
 	unregister_sysctl_table(hv_ctl_table_hdr);
 	hv_ctl_table_hdr = NULL;
 
@@ -315,6 +337,7 @@ u8 __init get_vtl(void)
 int __init hv_common_init(void)
 {
 	int i;
+	int ret;
 	union hv_hypervisor_version_info version;
 
 	/* Get information about the Microsoft Hypervisor version */
@@ -394,6 +417,13 @@ int __init hv_common_init(void)
 	for (i = 0; i < nr_cpu_ids; i++)
 		hv_vp_index[i] = VP_INVAL;
 
+	ret = rhashtable_init(&hv_pci_bus_ht, &hv_pci_bus_ht_params);
+	if (ret) {
+		hv_common_free();
+		return ret;
+	}
+	hv_pci_bus_ht_initialized = true;
+
 	return 0;
 }
 
@@ -808,6 +838,7 @@ static const struct hv_status_info hv_status_infos[] = {
 	_STATUS_INFO(HV_STATUS_PROCESSOR_FEATURE_NOT_SUPPORTED,	-EOPNOTSUPP),
 	_STATUS_INFO(HV_STATUS_INVALID_LP_INDEX,		-EINVAL),
 	_STATUS_INFO(HV_STATUS_INVALID_REGISTER_VALUE,		-EINVAL),
+	_STATUS_INFO(HV_STATUS_DEVICE_ALREADY_IN_DOMAIN,	-EEXIST),
 	_STATUS_INFO(HV_STATUS_OPERATION_FAILED,		-EIO),
 	_STATUS_INFO(HV_STATUS_TIME_OUT,			-EIO),
 	_STATUS_INFO(HV_STATUS_CALL_PENDING,			-EIO),
@@ -861,3 +892,87 @@ const char *hv_result_to_string(u64 status)
 	return "Unknown";
 }
 EXPORT_SYMBOL_GPL(hv_result_to_string);
+
+/*
+ * Logical device ID registry for Hyper-V PCI buses. The pci-hyperv
+ * driver registers each bus's logical device ID prefix before scanning
+ * its devices. Consumers look up the prefix by PCI domain number when
+ * building logical device IDs for Hyper-V interfaces.
+ */
+int hv_pci_register_dev_id(int pci_domain_nr, u32 logical_dev_id_prefix)
+{
+	struct hv_pci_busdata *bus, *new;
+	int ret;
+
+	new = kzalloc_obj(*new, GFP_KERNEL);
+	if (!new)
+		return -ENOMEM;
+
+	new->pci_domain_nr = pci_domain_nr;
+	new->logical_dev_id_prefix = logical_dev_id_prefix;
+
+	bus = rhashtable_lookup_get_insert_fast(&hv_pci_bus_ht, &new->node,
+						hv_pci_bus_ht_params);
+	if (IS_ERR(bus)) {
+		ret = PTR_ERR(bus);
+		goto free_new;
+	}
+
+	if (WARN_ONCE(bus != NULL,
+		      "Hyper-V PCI domain %04x is already registered\n",
+		      pci_domain_nr)) {
+		ret = -EEXIST;
+		goto free_new;
+	}
+
+	return 0;
+
+free_new:
+	kfree(new);
+	return ret;
+}
+EXPORT_SYMBOL_FOR_MODULES(hv_pci_register_dev_id, "pci-hyperv");
+
+void hv_pci_unregister_dev_id(int pci_domain_nr)
+{
+	struct hv_pci_busdata *bus;
+	int ret = -ENOENT;
+
+	rcu_read_lock();
+	bus = rhashtable_lookup(&hv_pci_bus_ht, &pci_domain_nr,
+				hv_pci_bus_ht_params);
+	if (bus)
+		ret = rhashtable_remove_fast(&hv_pci_bus_ht, &bus->node,
+					     hv_pci_bus_ht_params);
+	rcu_read_unlock();
+
+	if (WARN_ONCE(ret,
+		      "Failed to unregister Hyper-V PCI domain %04x\n",
+		      pci_domain_nr))
+		return;
+
+	kfree_rcu(bus, rcu);
+}
+EXPORT_SYMBOL_FOR_MODULES(hv_pci_unregister_dev_id, "pci-hyperv");
+
+/*
+ * Look up the logical device ID prefix registered for
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`find_hv_status_info`
- 外部函数：`hv_result`
- 外部函数：`unlikely`
- 外部函数：`unregister_sysctl_table`
- 大写宏：`ARRAY_SIZE`
- 大写宏：`EINVAL`
- 大写宏：`EIO`
- 大写宏：`EOPNOTSUPP`
- 大写宏：`EXPORT_SYMBOL_GPL`
- 大写宏：`HV_STATUS_CALL_PENDING`
- 大写宏：`HV_STATUS_INVALID_LP_INDEX`
- 大写宏：`HV_STATUS_INVALID_REGISTER_VALUE`
- 大写宏：`HV_STATUS_OPERATION_FAILED`
- 大写宏：`HV_STATUS_PROCESSOR_FEATURE_NOT_SUPPORTED`
- 大写宏：`HV_STATUS_TIME_OUT`
- 大写宏：`NULL`
- 大写宏：`U64_MAX`
- 大写宏：`VP_INVAL`
- 外部类型：`Called`
- 外部类型：`Get`
- 外部类型：`Hyper`
- 外部类型：`Hypercall`
- 外部类型：`Hypervisor`
- 外部类型：`Microsoft`
- 外部类型：`Unknown`
- 外部类型：`hv_hypervisor_version_info`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-813b0388bf` → 本草稿移入 `cases/defect/auto-linux-813b0388bf/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
