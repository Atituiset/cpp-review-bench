# auto-linux-859fd095bf

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#6b4b543fa18757297ac756a6fe6e64f506b7844a](https://github.com/torvalds/linux/commit/6b4b543fa18757297ac756a6fe6e64f506b7844a) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-26 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 16 |
| 编译错误数（gcc syntax-only） | 16（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #6b4b543fa18757297ac756a6fe6e64f506b7844a (https://github.com/torvalds/linux/commit/6b4b543fa18757297ac756a6fe6e64f506b7844a)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 426；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 6b4b543fa18757297ac756a6fe6e64f506b7844a Merge tag 'parisc-for-7.4-rc1' of git://git.kernel.org/pub/scm/linux/kernel/git/deller/parisc-linux :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -423,36 +423,9 @@ static inline pte_t pte_swp_clear_exclusive(pte_t pte)
 	return pte;
 }
 
-static inline pte_t ptep_get(pte_t *ptep)
-{
-	return READ_ONCE(*ptep);
-}
-#define ptep_get ptep_get
-
-static inline bool ptep_test_and_clear_young(struct vm_area_struct *vma,
-		unsigned long addr, pte_t *ptep)
-{
-	pte_t pte;
-
-	pte = ptep_get(ptep);
-	if (!pte_young(pte)) {
-		return false;
-	}
-	set_pte_at(vma->vm_mm, addr, ptep, pte_mkold(pte));
-	return true;
-}
-
 bool ptep_clear_flush_young(struct vm_area_struct *vma, unsigned long addr, pte_t *ptep);
 pte_t ptep_clear_flush(struct vm_area_struct *vma, unsigned long addr, pte_t *ptep);
 
-struct mm_struct;
-static inline void ptep_set_wrprotect(struct mm_struct *mm, unsigned long addr, pte_t *ptep)
-{
-	set_pte_at(mm, addr, ptep, pte_wrprotect(*ptep));
-}
-
-#define pte_same(A,B)	(pte_val(A) == pte_val(B))
-
 #endif /* !__ASSEMBLER__ */
 
 
@@ -482,10 +455,7 @@ static inline void ptep_set_wrprotect(struct mm_struct *mm, unsigned long addr,
 #define HAVE_ARCH_UNMAPPED_AREA
 #define HAVE_ARCH_UNMAPPED_AREA_TOPDOWN
 
-#define __HAVE_ARCH_PTEP_TEST_AND_CLEAR_YOUNG
 #define __HAVE_ARCH_PTEP_CLEAR_YOUNG_FLUSH
 #define __HAVE_ARCH_PTEP_CLEAR_FLUSH
-#define __HAVE_ARCH_PTEP_SET_WRPROTECT
-#define __HAVE_ARCH_PTE_SAME
 
 #endif /* _PARISC_PGTABLE_H */
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`_PAGE_ACCESSED`
- 外部函数：`_PAGE_WRITE`
- 外部函数：`pte_same`
- 外部函数：`pte_val`
- 外部函数：`pte_young`
- 外部函数：`ptep_clear_flush`
- 外部函数：`ptep_clear_flush_young`
- 外部函数：`set_pte_at`
- 外部函数：`set_ptes`
- 大写宏：`READ_ONCE`
- 外部类型：`Accessed`
- 外部类型：`Page`
- 外部类型：`Software`
- 外部类型：`mm_struct`
- 外部类型：`pte_t`
- 外部类型：`vm_area_struct`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-859fd095bf` → 本草稿移入 `cases/defect/auto-linux-859fd095bf/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
