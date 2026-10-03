# auto-linux-f5838393ec

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#8f150ccedfbd610aa25509ff42365d70fb20478f](https://github.com/torvalds/linux/commit/8f150ccedfbd610aa25509ff42365d70fb20478f) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-03 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 123 |
| 编译错误数（gcc syntax-only） | 3（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #8f150ccedfbd610aa25509ff42365d70fb20478f (https://github.com/torvalds/linux/commit/8f150ccedfbd610aa25509ff42365d70fb20478f)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 699；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 8f150ccedfbd610aa25509ff42365d70fb20478f Merge tag 'bpf-fixes' of git://git.kernel.org/pub/scm/linux/kernel/git/bpf/bpf :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -602,14 +602,18 @@ int arch_protect_bpf_trampoline(void *image, unsigned int size)
 }
 
 static int invoke_bpf_prog(u32 *image, u32 *ro_image, struct codegen_context *ctx,
-			   struct bpf_tramp_node *n, int regs_off, int retval_off,
-			   int run_ctx_off, bool save_ret)
+			   struct bpf_tramp_image *im, struct bpf_tramp_node *n,
+			   int regs_off, int retval_off, int run_ctx_off, bool save_ret)
 {
 	struct bpf_prog *p = n->link->prog;
 	ppc_inst_t branch_insn;
-	u32 jmp_idx;
+	u32 jmp_idx, skip_idx;
 	int ret = 0;
 
+	/* nop, patched to skip this prog when it is detached */
+	skip_idx = ctx->idx;
+	EMIT(PPC_RAW_NOP());
+
 	/* Save cookie */
 	if (IS_ENABLED(CONFIG_PPC64)) {
 		PPC_LI64(_R3, n->cookie);
@@ -679,13 +683,17 @@ static int invoke_bpf_prog(u32 *image, u32 *ro_image, struct codegen_context *ct
 	EMIT(PPC_RAW_ADDI(_R5, _R1, run_ctx_off));
 	ret = bpf_jit_emit_func_call_rel(image, ro_image, ctx,
 					 (unsigned long)bpf_trampoline_exit(p));
+	if (ret)
+		return ret;
 
-	return ret;
+	if (ro_image) /* image is NULL for dummy pass */
+		bpf_tramp_image_add_skip(im, p, &ro_image[skip_idx], &ro_image[ctx->idx]);
+	return 0;
 }
 
 static int invoke_bpf_mod_ret(u32 *image, u32 *ro_image, struct codegen_context *ctx,
-			      struct bpf_tramp_nodes *tn, int regs_off, int retval_off,
-			      int run_ctx_off, u32 *branches)
+			      struct bpf_tramp_image *im, struct bpf_tramp_nodes *tn,
+			      int regs_off, int retval_off, int run_ctx_off, u32 *branches)
 {
 	int i;
 
@@ -696,8 +704,8 @@ static int invoke_bpf_mod_ret(u32 *image, u32 *ro_image, struct codegen_context
 	EMIT(PPC_RAW_LI(_R3, 0));
 	EMIT(PPC_RAW_STL(_R3, _R1, retval_off));
 	for (i = 0; i < tn->nr_nodes; i++) {
-		if (invoke_bpf_prog(image, ro_image, ctx, tn->nodes[i], regs_off, retval_off,
-				    run_ctx_off, true))
+		if (invoke_bpf_prog(image, ro_image, ctx, im, tn->nodes[i], regs_off,
+				    retval_off, run_ctx_off, true))
 			return -EINVAL;
 
 		/*
@@ -1043,8 +1051,8 @@ static int __arch_prepare_bpf_trampoline(struct bpf_tramp_image *im, void *rw_im
 			cookie_ctx_off--;
 		}
 
-		if (invoke_bpf_prog(image, ro_image, ctx, fentry->nodes[i], regs_off, retval_off,
-				    run_ctx_off, flags & BPF_TRAMP_F_RET_FENTRY_RET))
+		if (invoke_bpf_prog(image, ro_image, ctx, im, fentry->nodes[i], regs_off,
+				    retval_off, run_ctx_off, flags & BPF_TRAMP_F_RET_FENTRY_RET))
 			return -EINVAL;
 	}
 
@@ -1053,7 +1061,7 @@ static int __arch_prepare_bpf_trampoline(struct bpf_tramp_image *im, void *rw_im
 		if (!branches)
 			return -ENOMEM;
 
-		if (invoke_bpf_mod_ret(image, ro_image, ctx, fmod_ret, regs_off, retval_off,
+		if (invoke_bpf_mod_ret(image, ro_image, ctx, im, fmod_ret, regs_off, retval_off,
 				       run_ctx_off, branches)) {
 			ret = -EINVAL;
 			goto cleanup;
@@ -1090,11 +1098,6 @@ static int __arch_prepare_bpf_trampoline(struct bpf_tramp_image *im, void *rw_im
 		/* Restore updated tail_call_cnt */
 		if (flags & BPF_TRAMP_F_TAIL_CALL_CTX)
 			bpf_trampoline_restore_tail_call_cnt(image, ctx, bpf_frame_size, r4_off);
-
-		/* Reserve space to patch branch instruction to skip fexit progs */
-		if (ro_image) /* image is NULL for dummy pass */
-			im->ip_after_call = &((u32 *)ro_image)[ctx->idx];
-		EMIT(PPC_RAW_NOP());
 	}
 
 	/* Update branches saved in invoke_bpf_mod_ret with address of do_fexit */
@@ -1123,16 +1126,14 @@ static int __arch_prepare_bpf_trampoline(struct bpf_tramp_image *im, void *rw_im
 			cookie_ctx_off--;
 		}
 
-		if (invoke_bpf_prog(image, ro_image, ctx, fexit->nodes[i], regs_off, retval_off,
-				    run_ctx_off, false)) {
+		if (invoke_bpf_prog(image, ro_image, ctx, im, fexit->nodes[i], regs_off,
+				    retval_off, run_ctx_off, false)) {
 			ret = -EINVAL;
 			goto cleanup;
 		}
 	}
 
 	if (flags & BPF_TRAMP_F_CALL_ORIG) {
-		if (ro_image) /* image is NULL for dummy pass */
-			im->ip_epilogue = &((u32 *)ro_image)[ctx->idx];
 		PPC_LI_ADDR(_R3, im);
 		ret = bpf_jit_emit_func_call_rel(image, ro_image, ctx,
 		
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`bpf_fsession_cnt`
- 外部函数：`bpf_jit_emit_func_call_rel`
- 外部函数：`bpf_jit_stack_offsetof`
- 外部函数：`bpf_trampoline_exit`
- 外部函数：`lr`
- 外部函数：`memset`
- 外部函数：`r4`
- 外部函数：`round_up`
- 大写宏：`BPF_PPC_TAILCALL`
- 大写宏：`BPF_PPC_TC`
- 大写宏：`BPF_TRAMP_FENTRY`
- 大写宏：`BPF_TRAMP_FEXIT`
- 大写宏：`BPF_TRAMP_MODIFY_RETURN`
- 大写宏：`COND_GT`
- 大写宏：`CONFIG_PPC32`
- 大写宏：`CONFIG_PPC64`
- 大写宏：`EINVAL`
- 大写宏：`EMIT`
- 大写宏：`EOPNOTSUPP`
- 大写宏：`IS_ENABLED`
- 大写宏：`MAX_BPF_FUNC_ARGS`
- 大写宏：`MAX_TAIL_CALL_CNT`
- 大写宏：`NULL`
- 大写宏：`PPC_BCC_CONST_SHORT`
- 大写宏：`PPC_LI64`
- 大写宏：`PPC_RAW_ADDI`
- 大写宏：`PPC_RAW_CMPLWI`
- 大写宏：`PPC_RAW_LD`
- 大写宏：`PPC_RAW_LI`
- 大写宏：`PPC_RAW_LL`
- 大写宏：`PPC_RAW_STL`
- 大写宏：`STACK_FRAME_MIN_SIZE`
- 大写宏：`SZL`
- 外部类型：`Extra`
- 外部类型：`Generated`
- 外部类型：`Refer`
- 外部类型：`Restore`
- 外部类型：`Save`
- 外部类型：`See`
- 外部类型：`Setting`
- 外部类型：`Trampoline`
- 外部类型：`Update`
- 外部类型：`arguments`
- 外部类型：`bpf_prog`
- 外部类型：`bpf_tramp_image`
- 外部类型：`bpf_tramp_node`
- 外部类型：`bpf_tramp_nodes`
- 外部类型：`btf_func_model`
- 外部类型：`codegen_context`
- 外部类型：`ppc_inst_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-f5838393ec` → 本草稿移入 `cases/defect/auto-linux-f5838393ec/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
