# auto-linux-1ffd3ebdb5

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#7cdf91542e1ed03aa8f5ab029702b3abde5be6d5](https://github.com/torvalds/linux/commit/7cdf91542e1ed03aa8f5ab029702b3abde5be6d5) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-09-28 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 16 |
| 编译错误数（gcc syntax-only） | 2（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #7cdf91542e1ed03aa8f5ab029702b3abde5be6d5 (https://github.com/torvalds/linux/commit/7cdf91542e1ed03aa8f5ab029702b3abde5be6d5)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 19（原始 PR diff 行 933；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 7cdf91542e1ed03aa8f5ab029702b3abde5be6d5 Merge tag 'sched_ext-for-7.3-rc4-fixes-2' of git://git.kernel.org/pub/scm/linux/kernel/git/tj/sched_ext :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -912,30 +912,36 @@ bool scx_cmask_empty(const struct scx_cmask *m)
 /**
  * scx_bpf_cid_topo - Copy out per-cid topology info
  * @cid: cid to look up
- * @out__uninit: where to copy the topology info; fully written by this call
+ * @out: where to copy the topology info
+ * @out__sz: size of @out, the program's sizeof(struct scx_cid_topo)
  * @aux: implicit BPF argument to access bpf_prog_aux hidden from BPF progs
  *
- * Fill @out__uninit with the topology info for @cid. Trigger scx_error() if
- * @cid is out of range. If @cid is valid but in the no-topo section, all fields
- * are set to -1. All fields are also set to -1 when no cid tables have been
- * published yet, which a program may observe while racing the root enable.
+ * Fill @out with the topology info for @cid. Trigger scx_error() if @cid is out
+ * of range. If @cid is valid but in the no-topo section, all fields are set to
+ * -1. All fields are also set to -1 when no cid tables have been published yet,
+ * which a program may observe while racing the root enable.
+ *
+ * The program's struct may be older or newer than the kernel's. The smaller of
+ * @out__sz and the kernel's size is copied and the rest of @out is set to -1.
  */
-__bpf_kfunc void scx_bpf_cid_topo(s32 cid, struct scx_cid_topo *out__uninit,
+__bpf_kfunc void scx_bpf_cid_topo(s32 cid, struct scx_cid_topo *out, size_t out__sz,
 				  const struct bpf_prog_aux *aux)
 {
+	size_t len = min(out__sz, sizeof(*out));
 	struct scx_cid_topo *topo;
 	struct scx_sched *sch;
 
+	/* the error cases and fields the kernel lacks read as -1 */
+	memset(out, 0xff, out__sz);
+
 	guard(rcu)();
 
 	sch = scx_prog_sched(aux);
 	topo = rcu_dereference(scx_cid_topo);
-	if (unlikely(!sch) || !cid_valid(sch, cid) || unlikely(!topo)) {
-		*out__uninit = SCX_CID_TOPO_NEG;
+	if (unlikely(!sch) || !cid_valid(sch, cid) || unlikely(!topo))
 		return;
-	}
 
-	*out__uninit = topo[cid];
+	memcpy(out, &topo[cid], len);
 }
 
 __bpf_kfunc_end_defs();
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__bpf_kfunc_end_defs`
- 外部函数：`cid_valid`
- 外部函数：`guard`
- 外部函数：`rcu_dereference`
- 外部函数：`scx_bpf_cid_topo`
- 外部函数：`scx_prog_sched`
- 外部函数：`unlikely`
- 大写宏：`BPF`
- 外部类型：`All`
- 外部类型：`Copy`
- 外部类型：`Fill`
- 外部类型：`If`
- 外部类型：`Trigger`
- 外部类型：`bpf_prog_aux`
- 外部类型：`scx_cid_topo`
- 外部类型：`scx_sched`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-1ffd3ebdb5` → 本草稿移入 `cases/defect/auto-linux-1ffd3ebdb5/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
