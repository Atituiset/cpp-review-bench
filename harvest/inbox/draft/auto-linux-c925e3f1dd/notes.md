# auto-linux-c925e3f1dd

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
| 外部依赖数（dep_count） | 34 |
| 编译错误数（gcc syntax-only） | 44（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #a4da531b171c664a6cf42c5b82d1610cbdf14fe4 (https://github.com/torvalds/linux/commit/a4da531b171c664a6cf42c5b82d1610cbdf14fe4)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 2630；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT a4da531b171c664a6cf42c5b82d1610cbdf14fe4 Merge tag 'bpf-fixes' of git://git.kernel.org/pub/scm/linux/kernel/git/bpf/bpf :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2626,8 +2626,13 @@ static int do_jit(struct bpf_verifier_env *env, struct bpf_prog *bpf_prog, int *
 	 * Arg 6 goes into r9 register, not on stack.
 	 */
 	outgoing_rsp = out_stack_arg_cnt > 1 ? (out_stack_arg_cnt - 1) * 8 : 0;
+	/*
+	 * bpf_throw() sees rsp of this frame as it is at a call. The callee-saved
+	 * regs are above the outgoing args, or above r9, which is pushed around
+	 * every call when the stack is private.
+	 */
 	if (bpf_prog->aux->exception_boundary)
-		bpf_prog->aux->stack_arg_sp_adjust = outgoing_rsp;
+		bpf_prog->aux->stack_arg_sp_adjust = outgoing_rsp + (priv_frame_ptr ? 8 : 0);
 	emit_sub_rsp(&prog, outgoing_rsp);
 
 	if (arena_vm_start)
@@ -3436,18 +3441,6 @@ static int do_jit(struct bpf_verifier_env *env, struct bpf_prog *bpf_prog, int *
 				ip += emit_kfunc_arg_moves(fm, outgoing_arg_base -
 							   outgoing_rsp, &prog);
 			}
-			if (func == (u8 *)arch_bpf_timed_may_goto) {
-				u32 fp = priv_frame_ptr ? X86_REG_R9 : BPF_REG_FP;
-
-				/*
-				 * AX has the offset of count and timestamp
-				 * in the stack. Turn it into a pointer.
-				 * add r10, rbp or add r10, r9
-				 */
-				maybe_emit_mod(&prog, BPF_REG_AX, fp, true);
-				EMIT2(0x01, add_2reg(0xC0, BPF_REG_AX, fp));
-				ip += 3;
-			}
 			if (priv_frame_ptr) {
 				push_r9(&prog);
 				ip += 2;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`barrier`
- 外部函数：`bpf_jit_plan_arg_moves`
- 外部函数：`is_ereg`
- 外部函数：`is_imm8`
- 大写宏：`BIT`
- 大写宏：`BPF_B`
- 大写宏：`BPF_DW`
- 大写宏：`BPF_H`
- 大写宏：`BPF_JIT_ARG_TMP`
- 大写宏：`BPF_JIT_MAX_ARG_MOVES`
- 大写宏：`BPF_REG_1`
- 大写宏：`BPF_REG_2`
- 大写宏：`BPF_REG_5`
- 大写宏：`BPF_REG_7`
- 大写宏：`BPF_REG_8`
- 大写宏：`BPF_REG_9`
- 大写宏：`BPF_REG_AX`
- 大写宏：`BPF_REG_FP`
- 大写宏：`BPF_W`
- 大写宏：`BPL`
- 大写宏：`DIL`
- 大写宏：`DST`
- 大写宏：`MAX_BPF_JIT_REG`
- 大写宏：`SIL`
- 大写宏：`SRC`
- 外部类型：`Add`
- 外部类型：`And`
- 外部类型：`Arg`
- 外部类型：`Emit`
- 外部类型：`No`
- 外部类型：`Take`
- 外部类型：`Turn`
- 外部类型：`bpf_jit_arg_move`
- 外部类型：`btf_func_model`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-c925e3f1dd` → 本草稿移入 `cases/defect/auto-linux-c925e3f1dd/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
