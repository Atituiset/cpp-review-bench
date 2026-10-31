# auto-linux-31dd0dd47f

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#17fa96334e15c6b38e8d9dd3627f3a6c5ca034f8](https://github.com/torvalds/linux/commit/17fa96334e15c6b38e8d9dd3627f3a6c5ca034f8) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-31 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 36 |
| 编译错误数（gcc syntax-only） | 13（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #17fa96334e15c6b38e8d9dd3627f3a6c5ca034f8 (https://github.com/torvalds/linux/commit/17fa96334e15c6b38e8d9dd3627f3a6c5ca034f8)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 17fa96334e15c6b38e8d9dd3627f3a6c5ca034f8 Merge tag 'for-linus' of git://git.kernel.org/pub/scm/virt/kvm/kvm :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2003,8 +2003,40 @@ int test_kvm_device(u32 gic_dev_type)
 	return 0;
 }
 
+static void test_vgic_v3_its_with_vgic_v5(void)
+{
+	struct kvm_vm *vm = vm_create_barebones();
+	int gic_fd, its_fd, ret;
+
+	if (__kvm_test_create_device(vm, KVM_DEV_TYPE_ARM_VGIC_ITS)) {
+		pr_info("No GICv3 ITS support; skipping mixed-model test.\n");
+		kvm_vm_free(vm);
+		return;
+	}
+
+	gic_fd = kvm_create_device(vm, KVM_DEV_TYPE_ARM_VGIC_V5);
+	ret = __kvm_create_device(vm, KVM_DEV_TYPE_ARM_VGIC_ITS);
+	TEST_ASSERT(ret < 0 && errno == ENODEV,
+		    "GICv3 ITS created after GICv5 VGIC: ret=%d errno=%d",
+		    ret, errno);
+	close(gic_fd);
+	kvm_vm_free(vm);
+
+	vm = vm_create_barebones();
+	its_fd = kvm_create_device(vm, KVM_DEV_TYPE_ARM_VGIC_ITS);
+	ret = __kvm_create_device(vm, KVM_DEV_TYPE_ARM_VGIC_V5);
+	TEST_ASSERT(ret < 0 && errno == ENODEV,
+		    "GICv5 VGIC created after GICv3 ITS: ret=%d errno=%d",
+		    ret, errno);
+	close(its_fd);
+	kvm_vm_free(vm);
+}
+
 void run_tests(u32 gic_dev_type)
 {
+	pr_info("Test VGICv5 and GICv3 ITS model mismatch\n");
+	test_vgic_v3_its_with_vgic_v5();
+
 	pr_info("Test VGICv5 address attrs\n");
 	test_vgic_v5_addr_attrs();
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__kvm_device_attr_get`
- 外部函数：`__kvm_device_attr_set`
- 外部函数：`__kvm_has_device_attr`
- 外部函数：`__vcpu_run`
- 外部函数：`__vm_create`
- 外部函数：`close`
- 外部函数：`kvm_create_device`
- 外部函数：`kvm_device_attr_set`
- 外部函数：`kvm_has_device_attr`
- 外部函数：`kvm_vm_free`
- 外部函数：`pr_info`
- 外部函数：`vm_vcpu_add`
- 大写宏：`E2BIG`
- 大写宏：`EEXIST`
- 大写宏：`EINVAL`
- 大写宏：`ENXIO`
- 大写宏：`GIC`
- 大写宏：`IPA`
- 大写宏：`IRS`
- 大写宏：`KVM_DEV_ARM_VGIC_CTRL_INIT`
- 大写宏：`KVM_DEV_ARM_VGIC_GRP_ADDR`
- 大写宏：`KVM_DEV_ARM_VGIC_GRP_CTRL`
- 大写宏：`KVM_DEV_TYPE_ARM_VGIC_V5`
- 大写宏：`KVM_VGIC_V5_ADDR_TYPE_IRS`
- 大写宏：`NULL`
- 大写宏：`TEST_ASSERT`
- 大写宏：`VGIC_ADDR_UNDEF`
- 大写宏：`VM_SHAPE_DEFAULT`
- 外部类型：`Attempt`
- 外部类型：`Check`
- 外部类型：`GICv5`
- 外部类型：`Space`
- 外部类型：`Test`
- 外部类型：`Try`
- 外部类型：`VGICv5`
- 外部类型：`kvm_vcpu`
- 外部类型：`kvm_vm`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-31dd0dd47f` → 本草稿移入 `cases/defect/auto-linux-31dd0dd47f/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
