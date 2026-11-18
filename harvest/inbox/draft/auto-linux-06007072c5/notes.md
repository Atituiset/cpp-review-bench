# auto-linux-06007072c5

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#20eef29b7c8465ac98d7dd6d6f2ac07a8d8056ca](https://github.com/torvalds/linux/commit/20eef29b7c8465ac98d7dd6d6f2ac07a8d8056ca) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-18 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 1 |
| 编译错误数（gcc syntax-only） | 4（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #20eef29b7c8465ac98d7dd6d6f2ac07a8d8056ca (https://github.com/torvalds/linux/commit/20eef29b7c8465ac98d7dd6d6f2ac07a8d8056ca)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 137；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 20eef29b7c8465ac98d7dd6d6f2ac07a8d8056ca Merge tag 'arc-fixes-7.4' of git://git.kernel.org/pub/scm/linux/kernel/git/vgupta/arc :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -133,21 +133,26 @@ static inline void __raw_writeb(u8 b, volatile void __iomem *addr)
 #define __raw_writew __raw_writew
 static inline void __raw_writew(u16 s, volatile void __iomem *addr)
 {
+	/*
+	 * Not an "m" operand: GCC can pick a register-indexed address for it,
+	 * which ST doesn't support (GCC PR target/127718).
+	 */
 	__asm__ __volatile__(
-	"	stw%U1 %0, %1	\n"
+	"	stw %0, [%1]	\n"
 	:
-	: "r" (s), "m" (*(volatile u16 __force *)addr)
+	: "r" (s), "r" (addr)
 	: "memory");
 
 }
 
 #define __raw_writel __raw_writel
 static inline void __raw_writel(u32 w, volatile void __iomem *addr)
 {
+	/* See __raw_writew() above for why this doesn't use an "m" operand. */
 	__asm__ __volatile__(
-	"	st%U1 %0, %1	\n"
+	"	st %0, [%1]	\n"
 	:
-	: "r" (w), "m" (*(volatile u32 __force *)addr)
+	: "r" (w), "r" (addr)
 	: "memory");
 
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__volatile__`

- **src/ 是原始切片，不可直接编译**；移植时要补全上下文使其独立编译。
- `// <<< BUG ANCHOR` 标记在移植时必须删除，golden anchor 改用重写后真实代码行。

## accept 检查清单

- [ ] 编译通过（重写后的 src/ 可独立编译）
- [ ] golden anchor 真实存在于 src/
- [ ] 触发条件已用一句话复述（见「缺陷描述与触发条件」）
- [ ] license 策略已遵守（rewrite 仓代码已重写表达）
- [ ] `// <<< BUG ANCHOR` 标记已清除
- [ ] notes 三段式已补全（缺陷描述 / 移植要点 / 契约安全（contract 候选））

## 接受后流程（accept → case）

1. 完成上面检查清单后评论 `/case accept auto-linux-06007072c5` → 本草稿移入 `cases/defect/auto-linux-06007072c5/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
