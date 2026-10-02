# auto-linux-e368514603

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#a940b03cee1524c10c16e0f73ec878bbd36202a2](https://github.com/torvalds/linux/commit/a940b03cee1524c10c16e0f73ec878bbd36202a2) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-02 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 29 |
| 编译错误数（gcc syntax-only） | 50（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #a940b03cee1524c10c16e0f73ec878bbd36202a2 (https://github.com/torvalds/linux/commit/a940b03cee1524c10c16e0f73ec878bbd36202a2)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 10（原始 PR diff 行 244；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT a940b03cee1524c10c16e0f73ec878bbd36202a2 Merge tag 'for-linus' of git://git.kernel.org/pub/scm/virt/kvm/kvm :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -216,6 +216,7 @@ static void sync_debug_state(struct pkvm_hyp_vcpu *hyp_vcpu)
 static void flush_hyp_vcpu(struct pkvm_hyp_vcpu *hyp_vcpu)
 {
 	struct kvm_vcpu *host_vcpu = hyp_vcpu->host_vcpu;
+	u64 host_hcr_mask = PKVM_HCR_EL2_HOST_PVM;
 
 	fpsimd_sve_flush();
 	flush_debug_state(hyp_vcpu);
@@ -228,6 +229,7 @@ static void flush_hyp_vcpu(struct pkvm_hyp_vcpu *hyp_vcpu)
 	if (!pkvm_hyp_vcpu_is_protected(hyp_vcpu)) {
 		if (vcpu_get_flag(host_vcpu, PKVM_HOST_STATE_DIRTY))
 			flush_hyp_vcpu_state(hyp_vcpu);
+		host_hcr_mask = PKVM_HCR_EL2_HOST_NPVM;
 	} else {
 		hyp_vcpu->vcpu.arch.ctxt = host_vcpu->arch.ctxt;
 	}
@@ -241,9 +243,8 @@ static void flush_hyp_vcpu(struct pkvm_hyp_vcpu *hyp_vcpu)
 	 * trap-control bit, so it must flow to the hyp vCPU alongside TWI/TWE
 	 * for the vSError to be delivered. sync_hyp_vcpu() reflects it back.
 	 */
-	hyp_vcpu->vcpu.arch.hcr_el2 &= ~(HCR_TWI | HCR_TWE | HCR_VSE);
-	hyp_vcpu->vcpu.arch.hcr_el2 |= READ_ONCE(host_vcpu->arch.hcr_el2) &
-						 (HCR_TWI | HCR_TWE | HCR_VSE);
+	hyp_vcpu->vcpu.arch.hcr_el2 &= ~host_hcr_mask;
+	hyp_vcpu->vcpu.arch.hcr_el2 |= READ_ONCE(host_vcpu->arch.hcr_el2) & host_hcr_mask;
 
 	hyp_vcpu->vcpu.arch.iflags	= host_vcpu->arch.iflags;
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`ctxt_mdscr_el1`
- 外部函数：`host_data_ptr`
- 外部函数：`kvm_guest_owns_debug_regs`
- 外部函数：`kvm_host_owns_debug_regs`
- 外部函数：`pkvm_hyp_vcpu_is_protected`
- 外部函数：`sync_hyp_vcpu`
- 外部函数：`vcpu_get_flag`
- 大写宏：`CNTP_CTL_EL0`
- 大写宏：`CNTP_CVAL_EL0`
- 大写宏：`CNTVOFF_EL2`
- 大写宏：`CNTV_CTL_EL0`
- 大写宏：`CNTV_CVAL_EL0`
- 大写宏：`EL1`
- 大写宏：`FP_STATE_HOST_OWNED`
- 大写宏：`HCR_TWE`
- 大写宏：`HCR_TWI`
- 大写宏：`HCR_VSE`
- 大写宏：`MDSCR_EL1`
- 大写宏：`NR_SYS_REGS`
- 大写宏：`PKVM_HOST_STATE_DIRTY`
- 大写宏：`READ_ONCE`
- 大写宏：`TWE`
- 大写宏：`TWI`
- 大写宏：`VNCR`
- 外部类型：`Copy`
- 外部类型：`The`
- 外部类型：`kvm_vcpu`
- 外部类型：`pkvm_hyp_vcpu`
- 外部类型：`vcpu_sysreg`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-e368514603` → 本草稿移入 `cases/defect/auto-linux-e368514603/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
