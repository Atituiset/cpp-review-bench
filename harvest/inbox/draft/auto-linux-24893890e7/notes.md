# auto-linux-24893890e7

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#03741bcea0a9ec4c8f386fd5c6051c1a14070240](https://github.com/torvalds/linux/commit/03741bcea0a9ec4c8f386fd5c6051c1a14070240) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-14 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 65 |
| 编译错误数（gcc syntax-only） | 86（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #03741bcea0a9ec4c8f386fd5c6051c1a14070240 (https://github.com/torvalds/linux/commit/03741bcea0a9ec4c8f386fd5c6051c1a14070240)
- 候选初判 scenario: **cwe-415（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 03741bcea0a9ec4c8f386fd5c6051c1a14070240 Merge tag 'for-linus' of git://git.kernel.org/pub/scm/virt/kvm/kvm :: PR 修复动作推断：修复前释放/双重释放（加释放守卫）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -955,6 +955,7 @@ void avic_init_vmcb(struct vcpu_svm *svm, struct vmcb *vmcb);
 int avic_incomplete_ipi_interception(struct kvm_vcpu *vcpu);
 int avic_unaccelerated_access_interception(struct kvm_vcpu *vcpu);
 int avic_init_vcpu(struct vcpu_svm *svm);
+void avic_vcpu_free(struct kvm_vcpu *vcpu);
 void avic_vcpu_load(struct kvm_vcpu *vcpu, int cpu);
 void avic_vcpu_put(struct kvm_vcpu *vcpu);
 void avic_apicv_post_state_restore(struct kvm_vcpu *vcpu);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`avic_apicv_post_state_restore`
- 外部函数：`avic_incomplete_ipi_interception`
- 外部函数：`avic_init_vcpu`
- 外部函数：`avic_unaccelerated_access_interception`
- 外部函数：`avic_vcpu_load`
- 外部函数：`avic_vcpu_put`
- 外部函数：`completes`
- 外部函数：`off`
- 外部函数：`posting`
- 外部函数：`put`
- 外部函数：`to`
- 外部函数：`vCPU`
- 大写宏：`AVIC`
- 大写宏：`CONFIG_HYPERV`
- 大写宏：`CONFIG_KVM_HYPERV`
- 大写宏：`CPU`
- 大写宏：`GIF`
- 大写宏：`IPI`
- 大写宏：`IRET`
- 大写宏：`IRQ`
- 大写宏：`IRTE`
- 大写宏：`IS_ENABLED`
- 大写宏：`KVM`
- 大写宏：`L2_CFG`
- 大写宏：`MAX_INTERCEPT`
- 大写宏：`MSR`
- 大写宏：`NMI`
- 大写宏：`RFLAGS`
- 大写宏：`RIP`
- 大写宏：`SEV`
- 大写宏：`SNP`
- 大写宏：`VIRT_SPEC_CTRL`
- 大写宏：`VMCB`
- 大写宏：`VMSA`
- 外部类型：`Change`
- 外部类型：`Contains`
- 外部类型：`Guest`
- 外部类型：`IRQs`
- 外部类型：`IRTEs`
- 外部类型：`If`
- 外部类型：`Indicates`
- 外部类型：`IsRunning`
- 外部类型：`NMIs`
- 外部类型：`Note`
- 外部类型：`Page`
- 外部类型：`Per`
- 外部类型：`Physical`
- 外部类型：`Set`
- 外部类型：`State`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`True`
- 外部类型：`Used`
- 外部类型：`ghcb`
- 外部类型：`gpa_t`
- 外部类型：`hv_vmcb_enlightenments`
- 外部类型：`is`
- 外部类型：`kvm_host_map`
- 外部类型：`kvm_vcpu`
- 外部类型：`list_head`
- 外部类型：`mutex`
- 外部类型：`raw_spinlock_t`
- 外部类型：`sev_es_save_area`
- 外部类型：`uint64_t`
- 外部类型：`vmcb`
- 外部类型：`vmcb_seg`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-24893890e7` → 本草稿移入 `cases/defect/auto-linux-24893890e7/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
