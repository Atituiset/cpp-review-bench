# auto-linux-393fefcf24

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#42e225323067a222230068651cf886bd7fbb1fe2](https://github.com/torvalds/linux/commit/42e225323067a222230068651cf886bd7fbb1fe2) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-25 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 14 |
| 编译错误数（gcc syntax-only） | 3（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #42e225323067a222230068651cf886bd7fbb1fe2 (https://github.com/torvalds/linux/commit/42e225323067a222230068651cf886bd7fbb1fe2)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 303；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 42e225323067a222230068651cf886bd7fbb1fe2 Merge tag 'for-linus' of git://git.kernel.org/pub/scm/virt/kvm/kvm :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -38,6 +38,9 @@ unsigned int kvm_host_sve_max_vl;
  */
 static DEFINE_PER_CPU(struct pkvm_hyp_vcpu *, loaded_hyp_vcpu);
 
+/* Per VM table entry, the vcpu_idx of the last vCPU of that VM loaded on this CPU. */
+static DEFINE_PER_CPU(unsigned int, last_vcpu_ran[KVM_MAX_PVMS]);
+
 /*
  * The PKVM_HCR_EL2_HOST_{PVM,NPVM} bits of this value come from the host on
  * each entry, and a protected vCPU's TWI and TWE at vCPU load.
@@ -271,6 +274,7 @@ struct pkvm_hyp_vcpu *pkvm_load_hyp_vcpu(pkvm_handle_t handle,
 {
 	struct pkvm_hyp_vcpu *hyp_vcpu = NULL;
 	struct pkvm_hyp_vm *hyp_vm;
+	unsigned int *last_ran;
 
 	/* Cannot load a new vcpu without putting the old one first. */
 	if (__this_cpu_read(loaded_hyp_vcpu))
@@ -300,8 +304,18 @@ struct pkvm_hyp_vcpu *pkvm_load_hyp_vcpu(pkvm_handle_t handle,
 unlock:
 	hyp_spin_unlock(&vm_table_lock);
 
-	if (hyp_vcpu)
-		__this_cpu_write(loaded_hyp_vcpu, hyp_vcpu);
+	if (!hyp_vcpu)
+		return NULL;
+
+	__this_cpu_write(loaded_hyp_vcpu, hyp_vcpu);
+
+	/* TLBs and I-cache are private to each vCPU, see vcpu_prepare_mmu(). */
+	last_ran = this_cpu_ptr(&last_vcpu_ran[vm_handle_to_idx(handle)]);
+	if (*last_ran != vcpu_idx) {
+		__kvm_flush_cpu_context(hyp_vcpu->vcpu.arch.hw_mmu);
+		*last_ran = vcpu_idx;
+	}
+
 	return hyp_vcpu;
 }
 
@@ -668,6 +682,7 @@ static int find_free_vm_table_entry(void)
  */
 static int allocate_vm_table_entry(void)
 {
+	unsigned int cpu;
 	int idx;
 
 	hyp_assert_lock_held(&vm_table_lock);
@@ -686,6 +701,9 @@ static int allocate_vm_table_entry(void)
 
 	vm_table[idx] = RESERVED_ENTRY;
 
+	for (cpu = 0; cpu < hyp_nr_cpus; cpu++)
+		per_cpu(last_vcpu_ran[idx], cpu) = UINT_MAX;
+
 	return idx;
 }
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__this_cpu_read`
- 外部函数：`__this_cpu_write`
- 外部函数：`hyp_assert_lock_held`
- 外部函数：`hyp_spin_unlock`
- 大写宏：`DEFINE_PER_CPU`
- 大写宏：`NPVM`
- 大写宏：`NULL`
- 大写宏：`PKVM_HCR_EL2_HOST_`
- 大写宏：`PVM`
- 大写宏：`TWE`
- 大写宏：`TWI`
- 外部类型：`Cannot`
- 外部类型：`The`
- 外部类型：`pkvm_hyp_vcpu`
- 外部类型：`pkvm_hyp_vm`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-393fefcf24` → 本草稿移入 `cases/defect/auto-linux-393fefcf24/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
