# auto-linux-842ba76817

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
| 外部依赖数（dep_count） | 88 |
| 编译错误数（gcc syntax-only） | 53（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #8f150ccedfbd610aa25509ff42365d70fb20478f (https://github.com/torvalds/linux/commit/8f150ccedfbd610aa25509ff42365d70fb20478f)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 7（原始 PR diff 行 1699；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 8f150ccedfbd610aa25509ff42365d70fb20478f Merge tag 'bpf-fixes' of git://git.kernel.org/pub/scm/linux/kernel/git/bpf/bpf :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1578,6 +1578,24 @@ void *bpf_arch_text_copy(void *dst, void *src, size_t len)
 	return ret ? ERR_PTR(-EINVAL) : dst;
 }
 
+int arch_bpf_trampoline_skip(void *nop, void *target)
+{
+	u32 old_insn = INSN_NOP;
+	u32 new_insn = larch_insn_gen_b((unsigned long)nop, (unsigned long)target);
+	int ret;
+
+	if (memcmp(nop, &old_insn, LOONGARCH_INSN_SIZE))
+		return -EFAULT;
+
+	cpus_read_lock();
+	mutex_lock(&text_mutex);
+	ret = larch_insn_text_copy(nop, &new_insn, LOONGARCH_INSN_SIZE);
+	mutex_unlock(&text_mutex);
+	cpus_read_unlock();
+
+	return ret;
+}
+
 int bpf_arch_text_poke(void *ip, enum bpf_text_poke_type old_t,
 		       enum bpf_text_poke_type new_t, void *old_addr,
 		       void *new_addr)
@@ -1696,13 +1714,18 @@ static void restore_stk_args(struct jit_ctx *ctx, int nr_stk_args, int args_off,
 	}
 }
 
-static int invoke_bpf_prog(struct jit_ctx *ctx, struct bpf_tramp_node *n,
-			   int args_off, int retval_off, int run_ctx_off, bool save_ret)
+static int invoke_bpf_prog(struct jit_ctx *ctx, struct bpf_tramp_image *im,
+			   struct bpf_tramp_node *n, int args_off, int retval_off,
+			   int run_ctx_off, bool save_ret)
 {
 	int ret;
 	u32 *branch;
 	struct bpf_prog *p = n->link->prog;
 	int cookie_off = offsetof(struct bpf_tramp_run_ctx, bpf_cookie);
+	void *skip = ctx->ro_image + ctx->idx;
+
+	/* nop, patched to a b over this prog when it is detached */
+	emit_insn(ctx, nop);
 
 	if (n->cookie)
 		emit_store_stack_imm64(ctx, LOONGARCH_GPR_T1,
@@ -1755,13 +1778,17 @@ static int invoke_bpf_prog(struct jit_ctx *ctx, struct bpf_tramp_node *n,
 	/* arg3: &run_ctx */
 	emit_insn(ctx, addid, LOONGARCH_GPR_A2, LOONGARCH_GPR_FP, -run_ctx_off);
 	ret = emit_call(ctx, (const u64)bpf_trampoline_exit(p));
+	if (ret)
+		return ret;
 
-	return ret;
+	bpf_tramp_image_add_skip(im, p, skip, ctx->ro_image + ctx->idx);
+	return 0;
 }
 
-static int invoke_bpf(struct jit_ctx *ctx, struct bpf_tramp_nodes *tn,
-		      int args_off, int retval_off, int run_ctx_off,
-		      int func_meta_off, bool save_ret, u64 func_meta, int cookie_off)
+static int invoke_bpf(struct jit_ctx *ctx, struct bpf_tramp_image *im,
+		      struct bpf_tramp_nodes *tn, int args_off, int retval_off,
+		      int run_ctx_off, int func_meta_off, bool save_ret,
+		      u64 func_meta, int cookie_off)
 {
 	int i, cur_cookie = (cookie_off - args_off) / 8;
 
@@ -1774,7 +1801,8 @@ static int invoke_bpf(struct jit_ctx *ctx, struct bpf_tramp_nodes *tn,
 			emit_store_stack_imm64(ctx, LOONGARCH_GPR_T1, -func_meta_off, meta);
 			cur_cookie--;
 		}
-		err = invoke_bpf_prog(ctx, tn->nodes[i], args_off, retval_off, run_ctx_off, save_ret);
+		err = invoke_bpf_prog(ctx, im, tn->nodes[i], args_off, retval_off,
+				      run_ctx_off, save_ret);
 		if (err)
 			return err;
 	}
@@ -2017,7 +2045,7 @@ static int __arch_prepare_bpf_trampoline(struct jit_ctx *ctx, struct bpf_tramp_i
 	}
 
 	if (fentry->nr_nodes) {
-		ret = invoke_bpf(ctx, fentry, args_off, retval_off, run_ctx_off, func_meta_off,
+		ret = invoke_bpf(ctx, im, fentry, args_off, retval_off, run_ctx_off, func_meta_off,
 				 flags & BPF_TRAMP_F_RET_FENTRY_RET, func_meta, cookie_off);
 		if (ret)
 			return ret;
@@ -2029,7 +2057,7 @@ static int __arch_prepare_bpf_trampoline(struct jit_ctx *ctx, struct bpf_tramp_i
 
 		emit_insn(ctx, std, LOONGARCH_GPR_ZERO, LOONGARCH_GPR_FP, -retval_off);
 		for (i = 0; i < fmod_ret->nr_nodes; i++) {
-			ret = invoke_bpf_prog(ctx, fmod_ret->nodes[i],
+			ret = invoke_bpf_prog(ctx, im, fmod_ret->nodes[i],
 					      args_off, retval_off, run_ctx_off, true);
 			if (ret)
 				goto out;
@@ -2051,10 +2079,6 @@ static int __arch_prepare_bpf_trampoline(struct jit_ctx *ctx, struct bpf_tramp_i
 			goto out;
 		emit_insn(ctx, std, LOONGARCH_GPR_A0, LOONGARCH_GPR_FP, -retval_off);
 		emit_insn(ctx, std, regmap[BPF_REG_0], LOONGARCH_GPR_FP, -(retval_off - 8));
-		im->ip_after_call = ctx->ro_image + ctx->idx;
-		/* Reserve space for the move_imm + jirl instruction */
-		for (i = 0; i < LOONGARCH_LONG_JUMP
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__arch_prepare_bpf_trampoline`
- 外部函数：`bpf_address_lookup`
- 外部函数：`bpf_trampoline_exit`
- 外部函数：`cpus_read_lock`
- 外部函数：`cpus_read_unlock`
- 外部函数：`emit_insn`
- 外部函数：`larch_insn_text_copy`
- 外部函数：`memcmp`
- 外部函数：`move_imm`
- 外部函数：`move_reg`
- 外部函数：`mutex_lock`
- 外部函数：`mutex_unlock`
- 外部函数：`offsetof`
- 外部函数：`pr_err`
- 外部函数：`pr_warn`
- 大写宏：`ABI`
- 大写宏：`BPF_MOD_CALL`
- 大写宏：`EFAULT`
- 大写宏：`EINVAL`
- 大写宏：`ENOTSUPP`
- 大写宏：`ERR_PTR`
- 大写宏：`INSN_NOP`
- 大写宏：`KSYM_NAME_LEN`
- 大写宏：`LOONGARCH_GPR_A0`
- 大写宏：`LOONGARCH_GPR_A2`
- 大写宏：`LOONGARCH_GPR_A6`
- 大写宏：`LOONGARCH_GPR_FP`
- 大写宏：`LOONGARCH_GPR_RA`
- 大写宏：`LOONGARCH_GPR_T1`
- 大写宏：`LOONGARCH_GPR_ZERO`
- 大写宏：`LOONGARCH_INSN_SIZE`
- 大写宏：`NULL`
- 外部类型：`Only`
- 外部类型：`Since`
- 外部类型：`Skip`
- 外部类型：`bpf_prog`
- 外部类型：`bpf_text_poke_type`
- 外部类型：`bpf_tramp_image`
- 外部类型：`bpf_tramp_node`
- 外部类型：`bpf_tramp_nodes`
- 外部类型：`bpf_tramp_run_ctx`
- 外部类型：`btf_func_model`
- 外部类型：`jit_ctx`
- 外部类型：`loongarch_instruction`
- 外部类型：`new_t`
- 外部类型：`old_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-842ba76817` → 本草稿移入 `cases/defect/auto-linux-842ba76817/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
