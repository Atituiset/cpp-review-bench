# auto-linux-c13b75c8e5

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#a4da531b171c664a6cf42c5b82d1610cbdf14fe4](https://github.com/torvalds/linux/commit/a4da531b171c664a6cf42c5b82d1610cbdf14fe4) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-04 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 12 |
| 编译错误数（gcc syntax-only） | 4（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #a4da531b171c664a6cf42c5b82d1610cbdf14fe4 (https://github.com/torvalds/linux/commit/a4da531b171c664a6cf42c5b82d1610cbdf14fe4)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 2431；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT a4da531b171c664a6cf42c5b82d1610cbdf14fe4 Merge tag 'bpf-fixes' of git://git.kernel.org/pub/scm/linux/kernel/git/bpf/bpf :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1005,6 +1005,17 @@ static noinline int bpf_jit_insn(struct bpf_jit *jit, struct bpf_prog *fp,
 					(jit->prg - patch_brc) >> 1;
 			break;
 		}
+		if (insn_is_mov_percpu_addr(insn)) {
+			const u64 *percpu_offset = &get_lowcore()->percpu_offset;
+
+			if (dst_reg != src_reg)
+				/* lgr %dst,%src */
+				EMIT4(0xb9040000, dst_reg, src_reg);
+			/* ag %dst,percpu_offset */
+			EMIT6_DISP_LH(0xe3000000, 0x0008, dst_reg, REG_0, REG_0,
+				      (unsigned long)percpu_offset);
+			break;
+		}
 		switch (insn->off) {
 		case 0: /* DST = SRC */
 			/* lgr %dst,%src */
@@ -1866,9 +1877,9 @@ static noinline int bpf_jit_insn(struct bpf_jit *jit, struct bpf_prog *fp,
 		if ((void *)func == arch_bpf_timed_may_goto) {
 			/*
 			 * arch_bpf_timed_may_goto() has a special ABI: the
-			 * parameters are in BPF_REG_AX and BPF_REG_10; the
-			 * return value is in BPF_REG_AX; and all GPRs except
-			 * REG_W0, REG_W1, and BPF_REG_AX are callee-saved.
+			 * parameter is in BPF_REG_AX; the return value is in
+			 * BPF_REG_AX; and all GPRs except REG_W0, REG_W1, and
+			 * BPF_REG_AX are callee-saved.
 			 */
 
 			/* brasl %r0,func */
@@ -2428,8 +2439,6 @@ struct bpf_prog *bpf_int_jit_compile(struct bpf_verifier_env *env, struct bpf_pr
 	fp->jited_len = jit.size - cfi_get_offset();
 
 	if (!fp->is_func || extra_pass) {
-		for (int i = 0; i < fp->len; i++)
-			jit.addrs[i] -= cfi_get_offset();
 		bpf_prog_fill_jited_linfo(fp, jit.addrs + 1);
 free_addrs:
 		kvfree(jit.addrs);
@@ -3116,6 +3125,11 @@ bool bpf_jit_supports_timed_may_goto(void)
 	return true;
 }
 
+bool bpf_jit_supports_percpu_insn(void)
+{
+	return true;
+}
+
 bool bpf_jit_inlines_helper_call(s32 imm)
 {
 	switch (imm) {
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`arch_bpf_timed_may_goto`
- 外部函数：`bpf_prog_fill_jited_linfo`
- 外部函数：`cfi_get_offset`
- 外部函数：`kvfree`
- 大写宏：`ABI`
- 大写宏：`BPF_REG_10`
- 大写宏：`BPF_REG_AX`
- 大写宏：`DST`
- 大写宏：`REG_W0`
- 大写宏：`REG_W1`
- 大写宏：`SRC`
- 外部类型：`GPRs`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-c13b75c8e5` → 本草稿移入 `cases/defect/auto-linux-c13b75c8e5/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
