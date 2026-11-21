# auto-linux-98dfb31d99

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#84afebd853437e44cb71f718be11d90e4c1a59dd](https://github.com/torvalds/linux/commit/84afebd853437e44cb71f718be11d90e4c1a59dd) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-21 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 16 |
| 编译错误数（gcc syntax-only） | 18（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #84afebd853437e44cb71f718be11d90e4c1a59dd (https://github.com/torvalds/linux/commit/84afebd853437e44cb71f718be11d90e4c1a59dd)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 84afebd853437e44cb71f718be11d90e4c1a59dd Merge tag 'bpf-fixes' of git://git.kernel.org/pub/scm/linux/kernel/git/bpf/bpf :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -152,6 +152,12 @@ bool arch_uretprobe_is_alive(struct return_instance *ret, enum rp_check ctx,
 		return regs->sp < ret->stack;
 }
 
+bool arch_uprobe_regs_writable(int off, int size)
+{
+	/* From x0 to x30, sp, pc. */
+	return off + size <= offsetof(struct pt_regs, pstate);
+}
+
 unsigned long
 arch_uretprobe_hijack_return_addr(unsigned long trampoline_vaddr,
 				  struct pt_regs *regs)
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`force_sig`
- 外部函数：`get_user_gcs`
- 外部函数：`procedure_link_pointer`
- 外部函数：`procedure_link_pointer_set`
- 外部函数：`put_user_gcs`
- 外部函数：`read_sysreg_s`
- 外部函数：`task_gcs_el0_enabled`
- 大写宏：`GCS`
- 大写宏：`PAC`
- 大写宏：`SIGSEGV`
- 大写宏：`SYS_GCSPR_EL0`
- 外部类型：`Explicitly`
- 外部类型：`If`
- 外部类型：`Likely`
- 外部类型：`Replace`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-98dfb31d99` → 本草稿移入 `cases/defect/auto-linux-98dfb31d99/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
