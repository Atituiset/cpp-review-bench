# auto-linux-cca2d24297

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#eff8d2791c086388ba5bae36385afd9bc6f0507e](https://github.com/torvalds/linux/commit/eff8d2791c086388ba5bae36385afd9bc6f0507e) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-09-27 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 34 |
| 编译错误数（gcc syntax-only） | 19（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #eff8d2791c086388ba5bae36385afd9bc6f0507e (https://github.com/torvalds/linux/commit/eff8d2791c086388ba5bae36385afd9bc6f0507e)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 465；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT eff8d2791c086388ba5bae36385afd9bc6f0507e Merge tag 'for-linus' of git://git.kernel.org/pub/scm/virt/kvm/kvm :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -360,7 +360,7 @@ static void pkvm_init_features_from_host(struct pkvm_hyp_vm *hyp_vm, const struc
 		if (test_bit(KVM_ARCH_FLAG_WRITABLE_IMP_ID_REGS, &host_arch_flags))
 			hyp_vm->kvm.arch.midr_el1 = host_kvm->arch.midr_el1;
 
-		return;
+		goto out;
 	}
 
 	if (kvm_pkvm_ext_allowed(kvm, KVM_CAP_ARM_MTE))
@@ -379,13 +379,14 @@ static void pkvm_init_features_from_host(struct pkvm_hyp_vm *hyp_vm, const struc
 	if (kvm_pkvm_ext_allowed(kvm, KVM_CAP_ARM_PTRAUTH_GENERIC))
 		set_bit(KVM_ARM_VCPU_PTRAUTH_GENERIC, allowed_features);
 
-	if (kvm_pkvm_ext_allowed(kvm, KVM_CAP_ARM_SVE)) {
+	if (kvm_pkvm_ext_allowed(kvm, KVM_CAP_ARM_SVE))
 		set_bit(KVM_ARM_VCPU_SVE, allowed_features);
-		kvm->arch.flags |= host_arch_flags & BIT(KVM_ARCH_FLAG_GUEST_HAS_SVE);
-	}
 
 	bitmap_and(kvm->arch.vcpu_features, host_kvm->arch.vcpu_features,
 		   allowed_features, KVM_VCPU_MAX_FEATURES);
+out:
+	__assign_bit(KVM_ARCH_FLAG_GUEST_HAS_SVE, &kvm->arch.flags,
+		     kvm_vcpu_has_feature(kvm, KVM_ARM_VCPU_SVE));
 }
 
 static void unpin_host_vcpu(struct kvm_vcpu *host_vcpu)
@@ -398,10 +399,10 @@ static void unpin_host_sve_state(struct pkvm_hyp_vcpu *hyp_vcpu)
 {
 	void *sve_state;
 
-	if (!vcpu_has_feature(&hyp_vcpu->vcpu, KVM_ARM_VCPU_SVE))
+	sve_state = hyp_vcpu->vcpu.arch.sve_state;
+	if (!sve_state)
 		return;
 
-	sve_state = hyp_vcpu->vcpu.arch.sve_state;
 	hyp_unpin_shared_mem(sve_state,
 			     sve_state + vcpu_sve_state_size(&hyp_vcpu->vcpu));
 }
@@ -450,7 +451,7 @@ static int pkvm_vcpu_init_sve(struct pkvm_hyp_vcpu *hyp_vcpu, struct kvm_vcpu *h
 	unsigned int sve_max_vl;
 	size_t sve_state_size;
 	void *sve_state;
-	int ret = 0;
+	int ret;
 
 	if (!vcpu_has_feature(vcpu, KVM_ARM_VCPU_SVE)) {
 		vcpu_clear_flag(vcpu, VCPU_SVE_FINALIZED);
@@ -459,25 +460,21 @@ static int pkvm_vcpu_init_sve(struct pkvm_hyp_vcpu *hyp_vcpu, struct kvm_vcpu *h
 
 	/* Limit guest vector length to the maximum supported by the host. */
 	sve_max_vl = min(READ_ONCE(host_vcpu->arch.sve_max_vl), kvm_host_sve_max_vl);
-	sve_state_size = sve_state_size_from_vl(sve_max_vl);
 	sve_state = kern_hyp_va(READ_ONCE(host_vcpu->arch.sve_state));
 
-	if (!sve_state || !sve_state_size) {
-		ret = -EINVAL;
-		goto err;
-	}
+	if (!sve_vl_valid(sve_max_vl) || !sve_state)
+		return -EINVAL;
+
+	sve_state_size = sve_state_size_from_vl(sve_max_vl);
 
 	ret = hyp_pin_shared_mem(sve_state, sve_state + sve_state_size);
 	if (ret)
-		goto err;
+		return ret;
 
 	vcpu->arch.sve_state = sve_state;
 	vcpu->arch.sve_max_vl = sve_max_vl;
 
 	return 0;
-err:
-	clear_bit(KVM_ARM_VCPU_SVE, vcpu->kvm->arch.vcpu_features);
-	return ret;
 }
 
 static int vm_copy_id_regs(struct pkvm_hyp_vcpu *hyp_vcpu)
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`bitmap_and`
- 外部函数：`clear_bit`
- 外部函数：`hyp_pin_shared_mem`
- 外部函数：`hyp_unpin_shared_mem`
- 外部函数：`kern_hyp_va`
- 外部函数：`kvm_pkvm_ext_allowed`
- 外部函数：`memcpy`
- 外部函数：`min`
- 外部函数：`pkvm_hyp_vcpu_to_hyp_vm`
- 外部函数：`set_bit`
- 外部函数：`sve_state_size_from_vl`
- 外部函数：`test_and_set_bit`
- 外部函数：`test_bit`
- 外部函数：`unpin_host_vcpu`
- 外部函数：`vcpu_clear_flag`
- 外部函数：`vcpu_has_feature`
- 外部函数：`vcpu_sve_state_size`
- 大写宏：`BIT`
- 大写宏：`EINVAL`
- 大写宏：`KVM_ARCH_FLAG_GUEST_HAS_SVE`
- 大写宏：`KVM_ARCH_FLAG_ID_REGS_INITIALIZED`
- 大写宏：`KVM_ARCH_FLAG_WRITABLE_IMP_ID_REGS`
- 大写宏：`KVM_ARM_VCPU_PTRAUTH_GENERIC`
- 大写宏：`KVM_ARM_VCPU_SVE`
- 大写宏：`KVM_CAP_ARM_MTE`
- 大写宏：`KVM_CAP_ARM_PTRAUTH_GENERIC`
- 大写宏：`KVM_CAP_ARM_SVE`
- 大写宏：`KVM_VCPU_MAX_FEATURES`
- 大写宏：`READ_ONCE`
- 大写宏：`VCPU_SVE_FINALIZED`
- 外部类型：`Limit`
- 外部类型：`kvm`
- 外部类型：`kvm_vcpu`
- 外部类型：`pkvm_hyp_vcpu`
- 外部类型：`pkvm_hyp_vm`
- 外部类型：`size_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-cca2d24297` → 本草稿移入 `cases/defect/auto-linux-cca2d24297/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
