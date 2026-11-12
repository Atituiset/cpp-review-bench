# auto-linux-70a2196a6d

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#79eb6679ef6a0d5ea68da53a8321fb1dc72299b9](https://github.com/torvalds/linux/commit/79eb6679ef6a0d5ea68da53a8321fb1dc72299b9) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-12 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 55 |
| 编译错误数（gcc syntax-only） | 20（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #79eb6679ef6a0d5ea68da53a8321fb1dc72299b9 (https://github.com/torvalds/linux/commit/79eb6679ef6a0d5ea68da53a8321fb1dc72299b9)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 79eb6679ef6a0d5ea68da53a8321fb1dc72299b9 Merge tag 'loongarch-fixes-7.4-1' of git://git.kernel.org/pub/scm/linux/kernel/git/chenhuacai/linux-loongson :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -535,6 +535,8 @@ u32 larch_insn_gen_lu32id(enum loongarch_gpr rd, int imm);
 u32 larch_insn_gen_lu52id(enum loongarch_gpr rd, enum loongarch_gpr rj, int imm);
 u32 larch_insn_gen_beq(enum loongarch_gpr rd, enum loongarch_gpr rj, int imm);
 u32 larch_insn_gen_bne(enum loongarch_gpr rd, enum loongarch_gpr rj, int imm);
+u32 larch_insn_gen_beqz(enum loongarch_gpr rj, int imm);
+u32 larch_insn_gen_bnez(enum loongarch_gpr rj, int imm);
 u32 larch_insn_gen_jirl(enum loongarch_gpr rd, enum loongarch_gpr rj, int imm);
 
 static inline bool signed_imm_check(long val, unsigned int bit)
@@ -592,6 +594,25 @@ DEF_EMIT_REG1I20_FORMAT(lu12iw, lu12iw_op)
 DEF_EMIT_REG1I20_FORMAT(lu32id, lu32id_op)
 DEF_EMIT_REG1I20_FORMAT(pcaddu18i, pcaddu18i_op)
 
+#define DEF_EMIT_REG1I21_FORMAT(NAME, OP)				\
+static inline void emit_##NAME(union loongarch_instruction *insn,	\
+			       enum loongarch_gpr rj, int offset)	\
+{									\
+	unsigned int immediate_l, immediate_h;				\
+									\
+	immediate_l = offset & 0xffff;					\
+	offset >>= 16;							\
+	immediate_h = offset & 0x1f;					\
+									\
+	insn->reg1i21_format.opcode = OP;				\
+	insn->reg1i21_format.immediate_l = immediate_l;			\
+	insn->reg1i21_format.rj = rj;					\
+	insn->reg1i21_format.immediate_h = immediate_h;			\
+}
+
+DEF_EMIT_REG1I21_FORMAT(beqz, beqz_op)
+DEF_EMIT_REG1I21_FORMAT(bnez, bnez_op)
+
 #define DEF_EMIT_REG2_FORMAT(NAME, OP)					\
 static inline void emit_##NAME(union loongarch_instruction *insn,	\
 			       enum loongarch_gpr rd,			\
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`larch_insn_gen_beq`
- 外部函数：`larch_insn_gen_bne`
- 外部函数：`larch_insn_gen_jirl`
- 外部函数：`larch_insn_gen_lu52id`
- 外部函数：`register`
- 大写宏：`CPU`
- 大写宏：`LOONGARCH_GPR_A0`
- 大写宏：`LOONGARCH_GPR_A1`
- 大写宏：`LOONGARCH_GPR_A2`
- 大写宏：`LOONGARCH_GPR_A3`
- 大写宏：`LOONGARCH_GPR_A4`
- 大写宏：`LOONGARCH_GPR_A5`
- 大写宏：`LOONGARCH_GPR_A6`
- 大写宏：`LOONGARCH_GPR_A7`
- 大写宏：`LOONGARCH_GPR_FP`
- 大写宏：`LOONGARCH_GPR_MAX`
- 大写宏：`LOONGARCH_GPR_RA`
- 大写宏：`LOONGARCH_GPR_S0`
- 大写宏：`LOONGARCH_GPR_S1`
- 大写宏：`LOONGARCH_GPR_S2`
- 大写宏：`LOONGARCH_GPR_S3`
- 大写宏：`LOONGARCH_GPR_S4`
- 大写宏：`LOONGARCH_GPR_S5`
- 大写宏：`LOONGARCH_GPR_S6`
- 大写宏：`LOONGARCH_GPR_S7`
- 大写宏：`LOONGARCH_GPR_S8`
- 大写宏：`LOONGARCH_GPR_SP`
- 大写宏：`LOONGARCH_GPR_T0`
- 大写宏：`LOONGARCH_GPR_T1`
- 大写宏：`LOONGARCH_GPR_T2`
- 大写宏：`LOONGARCH_GPR_T3`
- 大写宏：`LOONGARCH_GPR_T4`
- 大写宏：`LOONGARCH_GPR_T5`
- 大写宏：`LOONGARCH_GPR_T6`
- 大写宏：`LOONGARCH_GPR_T7`
- 大写宏：`LOONGARCH_GPR_T8`
- 大写宏：`LOONGARCH_GPR_TP`
- 大写宏：`LOONGARCH_GPR_U0`
- 大写宏：`LOONGARCH_GPR_ZERO`
- 大写宏：`NAME`
- 外部类型：`Kernel`
- 外部类型：`Reused`
- 外部类型：`reg0i15_format`
- 外部类型：`reg0i26_format`
- 外部类型：`reg1i20_format`
- 外部类型：`reg1i21_format`
- 外部类型：`reg2_format`
- 外部类型：`reg2bstrd_format`
- 外部类型：`reg2csr_format`
- 外部类型：`reg2i12_format`
- 外部类型：`reg2i14_format`
- 外部类型：`reg2i16_format`
- 外部类型：`reg2i5_format`
- 外部类型：`reg2i6_format`
- 外部类型：`reg3_format`
- 外部类型：`reg3sa2_format`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-70a2196a6d` → 本草稿移入 `cases/defect/auto-linux-70a2196a6d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
