# auto-linux-9df009a195

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
| 外部依赖数（dep_count） | 31 |
| 编译错误数（gcc syntax-only） | 58（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #8f150ccedfbd610aa25509ff42365d70fb20478f (https://github.com/torvalds/linux/commit/8f150ccedfbd610aa25509ff42365d70fb20478f)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 2421；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 8f150ccedfbd610aa25509ff42365d70fb20478f Merge tag 'bpf-fixes' of git://git.kernel.org/pub/scm/linux/kernel/git/bpf/bpf :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -2418,10 +2418,11 @@ bool bpf_jit_supports_subprog_tailcalls(void)
 	return true;
 }
 
-static void invoke_bpf_prog(struct jit_ctx *ctx, struct bpf_tramp_node *node,
-			    int bargs_off, int retval_off, int run_ctx_off,
-			    bool save_ret)
+static void invoke_bpf_prog(struct jit_ctx *ctx, struct bpf_tramp_image *im,
+			    struct bpf_tramp_node *node, int bargs_off,
+			    int retval_off, int run_ctx_off, bool save_ret)
 {
+	void *skip;
 	__le32 *branch;
 	u64 enter_prog;
 	u64 exit_prog;
@@ -2431,6 +2432,10 @@ static void invoke_bpf_prog(struct jit_ctx *ctx, struct bpf_tramp_node *node,
 	enter_prog = (u64)bpf_trampoline_enter(p);
 	exit_prog = (u64)bpf_trampoline_exit(p);
 
+	/* nop, patched to skip this prog when it is detached */
+	skip = ctx->ro_image + ctx->idx;
+	emit(A64_NOP, ctx);
+
 	if (node->cookie == 0) {
 		/* if cookie is zero, one instruction is enough to store it */
 		emit(A64_STR64I(A64_ZR, A64_SP, run_ctx_off + cookie_off), ctx);
@@ -2483,11 +2488,13 @@ static void invoke_bpf_prog(struct jit_ctx *ctx, struct bpf_tramp_node *node,
 	emit(A64_ADD_I(1, A64_R(2), A64_SP, run_ctx_off), ctx);
 
 	emit_call(exit_prog, ctx);
+
+	bpf_tramp_image_add_skip(im, p, skip, ctx->ro_image + ctx->idx);
 }
 
-static void invoke_bpf_mod_ret(struct jit_ctx *ctx, struct bpf_tramp_nodes *tn,
-			       int bargs_off, int retval_off, int run_ctx_off,
-			       __le32 **branches)
+static void invoke_bpf_mod_ret(struct jit_ctx *ctx, struct bpf_tramp_image *im,
+			       struct bpf_tramp_nodes *tn, int bargs_off,
+			       int retval_off, int run_ctx_off, __le32 **branches)
 {
 	int i;
 
@@ -2496,7 +2503,7 @@ static void invoke_bpf_mod_ret(struct jit_ctx *ctx, struct bpf_tramp_nodes *tn,
 	 */
 	emit(A64_STR64I(A64_ZR, A64_SP, retval_off), ctx);
 	for (i = 0; i < tn->nr_nodes; i++) {
-		invoke_bpf_prog(ctx, tn->nodes[i], bargs_off, retval_off,
+		invoke_bpf_prog(ctx, im, tn->nodes[i], bargs_off, retval_off,
 				run_ctx_off, true);
 		/* if (*(u64 *)(sp + retval_off) !=  0)
 		 *	goto do_fexit;
@@ -2882,7 +2889,7 @@ static int prepare_trampoline(struct jit_ctx *ctx, struct bpf_tramp_image *im,
 			store_func_meta(ctx, meta, func_meta_off);
 			cookie_bargs_off--;
 		}
-		invoke_bpf_prog(ctx, fentry->nodes[i], bargs_off,
+		invoke_bpf_prog(ctx, im, fentry->nodes[i], bargs_off,
 				retval_off, run_ctx_off,
 				flags & BPF_TRAMP_F_RET_FENTRY_RET);
 	}
@@ -2893,7 +2900,7 @@ static int prepare_trampoline(struct jit_ctx *ctx, struct bpf_tramp_image *im,
 		if (!branches)
 			return -ENOMEM;
 
-		invoke_bpf_mod_ret(ctx, fmod_ret, bargs_off, retval_off,
+		invoke_bpf_mod_ret(ctx, im, fmod_ret, bargs_off, retval_off,
 				   run_ctx_off, branches);
 	}
 
@@ -2906,9 +2913,6 @@ static int prepare_trampoline(struct jit_ctx *ctx, struct bpf_tramp_image *im,
 		emit(A64_RET(A64_R(10)), ctx);
 		/* store return value */
 		emit(A64_STR64I(A64_R(0), A64_SP, retval_off), ctx);
-		/* reserve a nop for bpf_tramp_image_put */
-		im->ip_after_call = ctx->ro_image + ctx->idx;
-		emit(A64_NOP, ctx);
 	}
 
 	/* update the branches saved in invoke_bpf_mod_ret with cbnz */
@@ -2930,12 +2934,11 @@ static int prepare_trampoline(struct jit_ctx *ctx, struct bpf_tramp_image *im,
 			store_func_meta(ctx, meta, func_meta_off);
 			cookie_bargs_off--;
 		}
-		invoke_bpf_prog(ctx, fexit->nodes[i], bargs_off, retval_off,
+		invoke_bpf_prog(ctx, im, fexit->nodes[i], bargs_off, retval_off,
 				run_ctx_off, false);
 	}
 
 	if (flags & BPF_TRAMP_F_CALL_ORIG) {
-		im->ip_epilogue = ctx->ro_image + ctx->idx;
 		/* for the first pass, assume the worst case */
 		if (!ctx->image)
 			ctx->idx += 4;
@@ -2994,7 +2997,7 @@ int arch_bpf_trampoline_size(const struct btf_func_model *m, u32 flags,
 		.image = NULL,
 		.idx = 0,
 	};
-	struct bpf_tramp_image im;
+	struct bpf_tramp_image im = {};
 	struct arg_aux aaux;
 	int ret;
 
@@ -3281,6 +3284,10 @@ int bpf_arch_text_poke(void *ip, enum bpf_text_poke_type old_t,
 	 *    longer reachable, since bpf_tramp_image_put()
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`aarch64_insn_gen_branch_imm`
- 外部函数：`bpf_trampoline_enter`
- 外部函数：`bpf_trampoline_exit`
- 外部函数：`cpu_to_le32`
- 外部函数：`fls64`
- 外部函数：`max`
- 外部函数：`round_down`
- 外部函数：`trampoline`
- 大写宏：`A64_ADD_I`
- 大写宏：`A64_BLR`
- 大写宏：`A64_MOVK`
- 大写宏：`A64_MOVN`
- 大写宏：`A64_MOVZ`
- 大写宏：`A64_R`
- 大写宏：`A64_RET`
- 大写宏：`A64_SP`
- 大写宏：`A64_STR64I`
- 大写宏：`A64_ZR`
- 大写宏：`AARCH64_INSN_BRANCH_LINK`
- 大写宏：`BPF_TRAMP_F_RET_FENTRY_RET`
- 大写宏：`ENOMEM`
- 大写宏：`MAX_BPF_JIT_REG`
- 大写宏：`NULL`
- 大写宏：`SZ_128M`
- 外部类型：`bpf_prog`
- 外部类型：`bpf_tramp_node`
- 外部类型：`bpf_tramp_nodes`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-9df009a195` → 本草稿移入 `cases/defect/auto-linux-9df009a195/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
