# auto-linux-a033dc233c

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#23a38e7bb2f9ad99c317a9f4e4c349054de9dfff](https://github.com/torvalds/linux/commit/23a38e7bb2f9ad99c317a9f4e4c349054de9dfff) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-19 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 11 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #23a38e7bb2f9ad99c317a9f4e4c349054de9dfff (https://github.com/torvalds/linux/commit/23a38e7bb2f9ad99c317a9f4e4c349054de9dfff)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 16；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 23a38e7bb2f9ad99c317a9f4e4c349054de9dfff Merge tag 'x86-urgent-2026-10-18' of git://git.kernel.org/pub/scm/linux/kernel/git/tip/tip :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -10,11 +10,11 @@
 #include <asm/insn-eval.h>
 
 /*
- * Returns the target address and the expected type when regs->ip points
+ * Returns the target address and the expected type when addr points
  * to a compiler-generated CFI trap.
  */
-static bool decode_cfi_insn(struct pt_regs *regs, unsigned long *target,
-			    u32 *type)
+static bool decode_cfi_insn(struct pt_regs *regs, unsigned long addr,
+			    unsigned long *target, u32 *type)
 {
 	char buffer[MAX_INSN_SIZE];
 	struct insn insn;
@@ -29,13 +29,13 @@ static bool decode_cfi_insn(struct pt_regs *regs, unsigned long *target,
 	 *   movl    -<id>, %r10d       ; 6 bytes
 	 *   addl    -<pos>(%reg), %r10d; 4 bytes
 	 *   je      .Ltmp1             ; 2 bytes
-	 *   ud2                        ; <- regs->ip
+	 *   ud2                        ; <- addr
 	 *   .Ltmp1:
 	 *
 	 * We can decode the expected type and the target address from the
 	 * movl/addl instructions.
 	 */
-	if (copy_from_kernel_nofault(buffer, (void *)regs->ip - 12, MAX_INSN_SIZE))
+	if (copy_from_kernel_nofault(buffer, (void *)addr - 12, MAX_INSN_SIZE))
 		return false;
 	if (insn_decode_kernel(&insn, &buffer[offset]))
 		return false;
@@ -44,7 +44,7 @@ static bool decode_cfi_insn(struct pt_regs *regs, unsigned long *target,
 
 	*type = -(u32)insn.immediate.value;
 
-	if (copy_from_kernel_nofault(buffer, (void *)regs->ip - 6, MAX_INSN_SIZE))
+	if (copy_from_kernel_nofault(buffer, (void *)addr - 6, MAX_INSN_SIZE))
 		return false;
 	if (insn_decode_kernel(&insn, &buffer[offset]))
 		return false;
@@ -82,7 +82,7 @@ enum bug_trap_type handle_cfi_failure(struct pt_regs *regs)
 				return BUG_TRAP_TYPE_NONE;
 		}
 
-		if (!decode_cfi_insn(regs, &target, &type))
+		if (!decode_cfi_insn(regs, addr, &target, &type))
 			return report_cfi_failure_noaddr(regs, addr);
 
 		break;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`copy_from_kernel_nofault`
- 外部函数：`insn_decode_kernel`
- 外部函数：`report_cfi_failure_noaddr`
- 大写宏：`BUG_TRAP_TYPE_NONE`
- 大写宏：`CFI`
- 大写宏：`MAX_INSN_SIZE`
- 外部类型：`Ltmp1`
- 外部类型：`Returns`
- 外部类型：`We`
- 外部类型：`insn`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-a033dc233c` → 本草稿移入 `cases/defect/auto-linux-a033dc233c/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
