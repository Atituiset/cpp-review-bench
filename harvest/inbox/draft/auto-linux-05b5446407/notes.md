# auto-linux-05b5446407

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
| 外部依赖数（dep_count） | 309 |
| 编译错误数（gcc syntax-only） | 88（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #a4da531b171c664a6cf42c5b82d1610cbdf14fe4 (https://github.com/torvalds/linux/commit/a4da531b171c664a6cf42c5b82d1610cbdf14fe4)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT a4da531b171c664a6cf42c5b82d1610cbdf14fe4 Merge tag 'bpf-fixes' of git://git.kernel.org/pub/scm/linux/kernel/git/bpf/bpf :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1752,12 +1752,22 @@ enum bpf_func_context {
 	BPF_FUNC_CTX_CNT,
 };
 
+struct bpf_subprog_arg_info {
+	enum bpf_arg_type arg_type;
+	union {
+		u32 mem_size;
+		u32 btf_id;
+	};
+};
+
 struct bpf_func_info_aux {
 	u16 linkage;
 	bool unreliable;
 	bool called[BPF_FUNC_CTX_CNT];
 	bool verified[BPF_FUNC_CTX_CNT];
 	bool called_with_pkt_ptrs;
+	/* what the callers of a global function are checked against */
+	struct bpf_subprog_arg_info args[MAX_BPF_FUNC_REG_ARGS];
 };
 
 enum bpf_jit_poke_reason {
@@ -1916,6 +1926,8 @@ struct bpf_prog_aux {
 	const char *attach_func_name;
 	struct bpf_prog **func;
 	struct bpf_prog_aux *main_prog_aux;
+	/* main prog: there when the prog has timed may_goto */
+	struct bpf_timed_may_goto __percpu *timed_may_goto;
 	void *jit_data; /* JIT specific data. arch dependent */
 	struct bpf_jit_poke_descriptor *poke_tab;
 	struct bpf_kfunc_desc_tab *kfunc_tab;
@@ -3044,6 +3056,8 @@ static inline void bpf_map_dec_elem_count(struct bpf_map *map)
 	this_cpu_dec(*map->elem_count);
 }
 
+s64 bpf_map_sum_elem_count(const struct bpf_map *map);
+
 extern int sysctl_unprivileged_bpf_disabled;
 
 bool bpf_token_capable(const struct bpf_token *token, int cap);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__poll_t`
- 外部函数：`bool`
- 外部函数：`btf_is_kernel`
- 外部函数：`int`
- 外部函数：`long`
- 外部函数：`u32`
- 外部函数：`u64`
- 外部函数：`userspace`
- 外部函数：`vm_fault_t`
- 外部函数：`void`
- 大写宏：`BPF_GRAPH_NODE`
- 大写宏：`BPF_GRAPH_ROOT`
- 大写宏：`BPF_KPTR`
- 大写宏：`BPF_KPTR_PERCPU`
- 大写宏：`BPF_KPTR_REF`
- 大写宏：`BPF_KPTR_UNREF`
- 大写宏：`BPF_LIST_HEAD`
- 大写宏：`BPF_LIST_NODE`
- 大写宏：`BPF_RB_NODE`
- 大写宏：`BPF_RB_ROOT`
- 大写宏：`BPF_RCU_HEAD`
- 大写宏：`BPF_REFCOUNT`
- 大写宏：`BPF_RES_SPIN_LOCK`
- 大写宏：`BPF_SPIN_LOCK`
- 大写宏：`BPF_TASK_WORK`
- 大写宏：`BPF_TIMER`
- 大写宏：`BPF_UPTR`
- 大写宏：`BPF_WORKQUEUE`
- 大写宏：`BTF`
- 大写宏：`NULL`
- 外部类型：`Direct`
- 外部类型：`Functions`
- 外部类型：`If`
- 外部类型：`It`
- 外部类型：`Misc`
- 外部类型：`Prog`
- 外部类型：`Some`
- 外部类型：`When`
- 外部类型：`allocated`
- 外部类型：`bpf_attr`
- 外部类型：`bpf_callback_t`
- 外部类型：`bpf_func_state`
- 外部类型：`bpf_insn`
- 外部类型：`bpf_iter_fini_seq_priv_t`
- 外部类型：`bpf_iter_init_seq_priv_t`
- 外部类型：`bpf_local_storage`
- 外部类型：`bpf_local_storage_map`
- 外部类型：`bpf_map`
- 外部类型：`bpf_prog`
- 外部类型：`bpf_prog_aux`
- 外部类型：`bpf_verifier_env`
- 外部类型：`btf`
- 外部类型：`btf_type`
- 外部类型：`file`
- 外部类型：`module`
- 外部类型：`poll_table_struct`
- 外部类型：`seq_file`
- 外部类型：`seq_operations`
- 外部类型：`vm_area_struct`
- 外部类型：`vm_fault`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-05b5446407` → 本草稿移入 `cases/defect/auto-linux-05b5446407/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
