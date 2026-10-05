# auto-linux-77710af189

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#06ac073129191a01d77933ed381f89c67674db6f](https://github.com/torvalds/linux/commit/06ac073129191a01d77933ed381f89c67674db6f) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-05 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 30 |
| 编译错误数（gcc syntax-only） | 2（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #06ac073129191a01d77933ed381f89c67674db6f (https://github.com/torvalds/linux/commit/06ac073129191a01d77933ed381f89c67674db6f)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 3（原始 PR diff 行 726；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 06ac073129191a01d77933ed381f89c67674db6f Merge tag 'x86-urgent-2026-10-04' of git://git.kernel.org/pub/scm/linux/kernel/git/tip/tip :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -705,47 +705,37 @@ int pmd_clear_huge(pmd_t *pmd)
 }
 
 #ifdef CONFIG_X86_64
-/**
- * pud_free_pmd_page - Clear PUD entry and free PMD page
- * @pud: Pointer to a PUD
- * @addr: Virtual address associated with PUD
- *
- * Context: The PUD range has been unmapped and TLB purged.
- * Return: 1 if clearing the entry succeeded. 0 otherwise.
- *
- * NOTE: Callers must allow a single page allocation.
+/*
+ * Given a PUD poitner, detach and free the pointed-to
+ * PMD page and any PTE page children. The entire range
+ * under the PUD must not have any valid translations
+ * and the TLB must have already been flushed.
  */
 int pud_free_pmd_page(pud_t *pud, unsigned long addr)
 {
-	pmd_t *pmd, *pmd_sv;
 	struct ptdesc *pt;
+	pmd_t *pmd;
 	int i;
 
 	pmd = pud_pgtable(*pud);
-	pmd_sv = (pmd_t *)__get_free_page(GFP_KERNEL);
-	if (!pmd_sv)
-		return 0;
-
-	for (i = 0; i < PTRS_PER_PMD; i++) {
-		pmd_sv[i] = pmd[i];
-		if (!pmd_none(pmd[i]))
-			pmd_clear(&pmd[i]);
-	}
 
+	/* Detach the PMD page: */
 	pud_clear(pud);
 
-	/* INVLPG to clear all paging-structure caches */
+	/*
+	 * PMD and all its descendents are unreachable
+	 * via normal page walks. Make them unreachable
+	 * in cached mid-level walks too:
+	 */
 	flush_tlb_kernel_range(addr, addr + PAGE_SIZE-1);
 
 	for (i = 0; i < PTRS_PER_PMD; i++) {
-		if (!pmd_none(pmd_sv[i])) {
-			pt = page_ptdesc(pmd_page(pmd_sv[i]));
+		if (!pmd_none(pmd[i])) {
+			pt = page_ptdesc(pmd_page(pmd[i]));
 			pagetable_dtor_free(pt);
 		}
 	}
 
-	free_page((unsigned long)pmd_sv);
-
 	pmd_free(&init_mm, pmd);
 
 	return 1;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__get_free_page`
- 外部函数：`flush_tlb_kernel_range`
- 外部函数：`free_page`
- 外部函数：`page_ptdesc`
- 外部函数：`pagetable_dtor_free`
- 外部函数：`pmd_clear`
- 外部函数：`pmd_free`
- 外部函数：`pmd_none`
- 外部函数：`pmd_page`
- 外部函数：`pud_clear`
- 外部函数：`pud_pgtable`
- 大写宏：`CONFIG_X86_64`
- 大写宏：`GFP_KERNEL`
- 大写宏：`INVLPG`
- 大写宏：`NOTE`
- 大写宏：`PAGE_SIZE`
- 大写宏：`PMD`
- 大写宏：`PTRS_PER_PMD`
- 大写宏：`PUD`
- 大写宏：`TLB`
- 外部类型：`Callers`
- 外部类型：`Clear`
- 外部类型：`Context`
- 外部类型：`Pointer`
- 外部类型：`Return`
- 外部类型：`The`
- 外部类型：`Virtual`
- 外部类型：`pmd_t`
- 外部类型：`ptdesc`
- 外部类型：`pud_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-77710af189` → 本草稿移入 `cases/defect/auto-linux-77710af189/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
