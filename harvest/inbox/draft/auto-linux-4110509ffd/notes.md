# auto-linux-4110509ffd

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#2681617eb9317444247380298564f58142fc4ff8](https://github.com/torvalds/linux/commit/2681617eb9317444247380298564f58142fc4ff8) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-23 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 42 |
| 编译错误数（gcc syntax-only） | 60（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #2681617eb9317444247380298564f58142fc4ff8 (https://github.com/torvalds/linux/commit/2681617eb9317444247380298564f58142fc4ff8)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 2681617eb9317444247380298564f58142fc4ff8 Merge tag 'riscv-for-linus-7.4-rc4' of git://git.kernel.org/pub/scm/linux/kernel/git/riscv/linux :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -320,6 +320,8 @@ static inline void riscv_v_vstate_save(struct __riscv_v_ext_state *vstate,
 static inline void riscv_v_vstate_restore(struct __riscv_v_ext_state *vstate,
 					  struct pt_regs *regs)
 {
+	WARN_ON_ONCE(preemptible());
+
 	if (riscv_v_vstate_query(regs)) {
 		__riscv_v_vstate_restore(vstate, vstate->datap);
 		__riscv_v_vstate_clean(regs);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__riscv_v_vstate_check`
- 外部函数：`__riscv_v_vstate_or`
- 外部函数：`__stringify`
- 外部函数：`csr_clear`
- 外部函数：`csr_read`
- 外部函数：`csr_read_set`
- 外部函数：`csr_set`
- 外部函数：`csr_write`
- 外部函数：`riscv_has_vendor_extension_unlikely`
- 外部函数：`typeof`
- 外部函数：`volatile`
- 大写宏：`CLEAN`
- 大写宏：`CONFIG_RISCV_ISA_XTHEADVECTOR`
- 大写宏：`CSR_SSTATUS`
- 大写宏：`CSR_STATUS`
- 大写宏：`CSR_VCSR`
- 大写宏：`CSR_VSTART`
- 大写宏：`CSR_VXRM`
- 大写宏：`CSR_VXRM_MASK`
- 大写宏：`CSR_VXRM_SHIFT`
- 大写宏：`CSR_VXSAT`
- 大写宏：`CSR_VXSAT_MASK`
- 大写宏：`IS_ENABLED`
- 大写宏：`OFF`
- 大写宏：`RISCV_ISA_VENDOR_EXT_XTHEADVECTOR`
- 大写宏：`SR_FS`
- 大写宏：`SR_FS_DIRTY`
- 大写宏：`SR_VS`
- 大写宏：`SR_VS_`
- 大写宏：`SR_VS_THEAD`
- 大写宏：`THEAD_VENDOR_ID`
- 大写宏：`THEAD_VLB_V_V0T0`
- 大写宏：`THEAD_VLB_V_V16T0`
- 大写宏：`THEAD_VLB_V_V24T0`
- 大写宏：`THEAD_VLB_V_V8T0`
- 大写宏：`THEAD_VSETVLI_T4X0E8M8D1`
- 大写宏：`TYPE`
- 大写宏：`VXRM`
- 大写宏：`VXSAT`
- 外部类型：`CSRs`
- 外部类型：`Similar`
- 外部类型：`__riscv_v_ext_state`
- 外部类型：`pt_regs`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-4110509ffd` → 本草稿移入 `cases/defect/auto-linux-4110509ffd/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
