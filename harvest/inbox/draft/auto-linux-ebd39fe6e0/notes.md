# auto-linux-ebd39fe6e0

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
| 外部依赖数（dep_count） | 74 |
| 编译错误数（gcc syntax-only） | 16（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #eff8d2791c086388ba5bae36385afd9bc6f0507e (https://github.com/torvalds/linux/commit/eff8d2791c086388ba5bae36385afd9bc6f0507e)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 97；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT eff8d2791c086388ba5bae36385afd9bc6f0507e Merge tag 'for-linus' of git://git.kernel.org/pub/scm/virt/kvm/kvm :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -45,11 +45,24 @@ struct vncr_tlb {
  */
 #define S2_MMU_PER_VCPU		2
 
-void kvm_init_nested(struct kvm *kvm)
+int kvm_init_nested(struct kvm *kvm)
 {
-	kvm->arch.nested_mmus = NULL;
+	kvm->arch.nested_mmus = kvmalloc_objs(struct kvm_s2_mmu *,
+					      KVM_MAX_VCPUS * S2_MMU_PER_VCPU,
+					      GFP_KERNEL_ACCOUNT);
 	kvm->arch.nested_mmus_size = 0;
 	atomic_set(&kvm->arch.vncr_tlb_count, 0);
+
+	return kvm->arch.nested_mmus ? 0 : -ENOMEM;
+}
+
+void kvm_destroy_nested(struct kvm *kvm)
+{
+	for (int i = 0; i < kvm->arch.nested_mmus_size; i+= S2_MMU_PER_VCPU)
+		kvfree(kvm->arch.nested_mmus[i]);
+
+	kvm->arch.nested_mmus_size = 0;
+	kvfree(kvm->arch.nested_mmus);
 }
 
 static int init_nested_s2_mmu(struct kvm *kvm, struct kvm_s2_mmu *mmu)
@@ -70,8 +83,9 @@ static int init_nested_s2_mmu(struct kvm *kvm, struct kvm_s2_mmu *mmu)
 int kvm_vcpu_init_nested(struct kvm_vcpu *vcpu)
 {
 	struct kvm *kvm = vcpu->kvm;
-	struct kvm_s2_mmu *tmp;
-	int num_mmus, ret = 0;
+	int num_mmus;
+
+	lockdep_assert_held(&kvm->arch.config_lock);
 
 	if (test_bit(KVM_ARM_VCPU_HAS_EL2_E2H0, kvm->arch.vcpu_features) &&
 	    !cpus_have_final_cap(ARM64_HAS_HCR_NV1))
@@ -84,51 +98,40 @@ int kvm_vcpu_init_nested(struct kvm_vcpu *vcpu)
 	if (!vcpu->arch.ctxt.vncr_array)
 		return -ENOMEM;
 
-	/*
-	 * Let's treat memory allocation failures as benign: If we fail to
-	 * allocate anything, return an error and keep the allocated array
-	 * alive. Userspace may try to recover by initializing the vcpu
-	 * again, and there is no reason to affect the whole VM for this.
-	 */
 	num_mmus = atomic_read(&kvm->online_vcpus) * S2_MMU_PER_VCPU;
 
 	if (num_mmus > kvm->arch.nested_mmus_size) {
-		tmp = kvzalloc_objs(*tmp, num_mmus, GFP_KERNEL_ACCOUNT);
-		if (!tmp)
-			return -ENOMEM;
+		struct kvm_s2_mmu *tmp;
+		int i, ret = 0;
 
-		write_lock(&kvm->mmu_lock);
-
-		if (kvm->arch.nested_mmus_size) {
-			memcpy(tmp, kvm->arch.nested_mmus,
-			       size_mul(sizeof(*tmp), kvm->arch.nested_mmus_size));
+		tmp = kvzalloc_objs(*tmp, S2_MMU_PER_VCPU, GFP_KERNEL_ACCOUNT);
+		if (!tmp)
+			ret = -ENOMEM;
 
-			for (int i = 0; i < kvm->arch.nested_mmus_size; i++)
-				tmp[i].pgt->mmu = &tmp[i];
+		for (i = 0; !ret && i < S2_MMU_PER_VCPU; i++) {
+			ret = init_nested_s2_mmu(kvm, &tmp[i]);
+			if (ret)
+				break;
 		}
 
-		swap(kvm->arch.nested_mmus, tmp);
-
-		write_unlock(&kvm->mmu_lock);
-
-		kvfree(tmp);
-	}
+		if (ret) {
+			while (--i >= 0)
+				kvm_free_stage2_pgd(&tmp[i]);
 
-	for (int i = kvm->arch.nested_mmus_size; !ret && i < num_mmus; i++)
-		ret = init_nested_s2_mmu(kvm, &kvm->arch.nested_mmus[i]);
+			kvfree(tmp);
+			free_page((unsigned long)vcpu->arch.ctxt.vncr_array);
+			vcpu->arch.ctxt.vncr_array = NULL;
+			return ret;
+		}
 
-	if (ret) {
-		for (int i = kvm->arch.nested_mmus_size; i < num_mmus; i++)
-			kvm_free_stage2_pgd(&kvm->arch.nested_mmus[i]);
+		guard(write_lock)(&kvm->mmu_lock);
 
-		free_page((unsigned long)vcpu->arch.ctxt.vncr_array);
-		vcpu->arch.ctxt.vncr_array = NULL;
+		for (i = 0; i < S2_MMU_PER_VCPU; i++)
+			kvm->arch.nested_mmus[i + kvm->arch.nested_mmus_size] = &tmp[i];
 
-		return ret;
+		kvm->arch.nested_mmus_size += S2_MMU_PER_VCPU;
 	}
 
-	kvm->arch.nested_mmus_size = num_mmus;
-
 	return 0;
 }
 
@@ -742,7 +745,7 @@ void kvm_s2_mmu_iterate_by_vmid(struct kvm *kvm, u16 vmid,
 	write_lock(&kvm->mmu_lock);
 
 	for (int i = 0; i < kvm->arch.nested_mmus_size; i++) {
-		struct kvm_s2_mmu *mmu = &kvm->arch.nested_mmus[i];
+		struct kvm_s2_mmu *mmu = kvm->arch.nested_mmus[i];
 
 		if (!kvm_s2_mmu_valid(mmu))
 			continue;
@@ -784,7 +787,7 @@ struct kvm_s2_mmu *lookup_s2_mmu(struct kvm_vcpu *vcpu)
 	 *   if S2 translation is disabled.
 	 */
 	for (int i = 0; i < kvm->arch.nested_mmus_size; i++) {
-		struct kvm_s2_mmu *mmu = &kvm->arch.nested_mmus[i];
+		struct kvm_s2_mmu *mmu = kvm->arch.nested_mmus[i];
 
 		if (!kvm_s2_mmu_valid(mmu))
 			continue;
@@ -823,7 +826,7 @@ static struct kvm_s2_mmu *get_s2_mmu_nested(struct kvm_vcpu *vcpu)
 	fo
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__get_free_page`
- 外部函数：`atomic_dec`
- 外部函数：`atomic_read`
- 外部函数：`atomic_set`
- 外部函数：`atomic_xchg_relaxed`
- 外部函数：`clear_fixmap`
- 外部函数：`cpus_have_final_cap`
- 外部函数：`free_page`
- 外部函数：`kvfree`
- 外部函数：`kvm_for_each_vncr_tlb`
- 外部函数：`kvm_free_stage2_pgd`
- 外部函数：`kvm_has_feat`
- 外部函数：`kvm_s2_mmu_valid`
- 外部函数：`kvm_vcpu_init_nested`
- 外部函数：`kvzalloc_objs`
- 外部函数：`lockdep_assert_held_write`
- 外部函数：`memcpy`
- 外部函数：`size_mul`
- 外部函数：`swap`
- 外部函数：`test_bit`
- 外部函数：`vncr_fixmap`
- 外部函数：`write_lock`
- 外部函数：`write_unlock`
- 大写宏：`ARM64_HAS_HCR_NV1`
- 大写宏：`BUG`
- 大写宏：`BUG_ON`
- 大写宏：`CPU`
- 大写宏：`EINVAL`
- 大写宏：`ENOMEM`
- 大写宏：`GFP_KERNEL_ACCOUNT`
- 大写宏：`ID_AA64MMFR4_EL1`
- 大写宏：`IPA`
- 大写宏：`KVM_ARM_VCPU_HAS_EL2_E2H0`
- 大写宏：`MMU`
- 大写宏：`NULL`
- 大写宏：`NV2_ONLY`
- 大写宏：`SZ_16K`
- 大写宏：`SZ_1G`
- 大写宏：`SZ_2M`
- 大写宏：`SZ_32M`
- 大写宏：`SZ_4K`
- 大写宏：`SZ_512M`
- 大写宏：`SZ_64K`
- 大写宏：`TLB`
- 大写宏：`TLBI_ALL`
- 大写宏：`TLBI_ASID`
- 大写宏：`TLBI_TTL_TG_16K`
- 大写宏：`TLBI_TTL_TG_4K`
- 大写宏：`TLBI_TTL_TG_64K`
- 大写宏：`TLBI_VA`
- 大写宏：`TLBI_VAA`
- 大写宏：`VNCR`
- 大写宏：`VNCR_EL2`
- 外部类型：`Can`
- 外部类型：`If`
- 外部类型：`Let`
- 外部类型：`NV_frac`
- 外部类型：`No`
- 外部类型：`Note`
- 外部类型：`The`
- 外部类型：`Userspace`
- 外部类型：`atomic_t`
- 外部类型：`kvm`
- 外部类型：`kvm_s2_mmu`
- 外部类型：`kvm_vcpu`
- 外部类型：`s1_walk_info`
- 外部类型：`s1_walk_result`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-ebd39fe6e0` → 本草稿移入 `cases/defect/auto-linux-ebd39fe6e0/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
