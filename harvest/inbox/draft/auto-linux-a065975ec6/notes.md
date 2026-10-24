# auto-linux-a065975ec6

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#906b6e1d8c782a8442b30bdf8b8992f5031ffc59](https://github.com/torvalds/linux/commit/906b6e1d8c782a8442b30bdf8b8992f5031ffc59) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-24 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 8 |
| 编译错误数（gcc syntax-only） | 6（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #906b6e1d8c782a8442b30bdf8b8992f5031ffc59 (https://github.com/torvalds/linux/commit/906b6e1d8c782a8442b30bdf8b8992f5031ffc59)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 1631；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 906b6e1d8c782a8442b30bdf8b8992f5031ffc59 Merge tag 'dma-mapping-7.4-2026-10-21' of git://git.kernel.org/pub/scm/linux/kernel/git/mszyprowski/linux :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -1628,10 +1628,12 @@ static void *iommu_dma_alloc_pages(struct device *dev, size_t size,
 	void *cpu_addr;
 
 	page = dma_alloc_contiguous(dev, alloc_size, gfp);
-	if (!page)
-		page = alloc_pages_node(node, gfp, get_order(alloc_size));
-	if (!page)
-		return NULL;
+	if (!page) {
+		cpu_addr = alloc_pages_exact_nid(node, alloc_size, gfp);
+		if (!cpu_addr)
+			return NULL;
+		page = virt_to_page(cpu_addr);
+	}
 
 	if (!coherent || PageHighMem(page)) {
 		pgprot_t prot = dma_pgprot(dev, PAGE_KERNEL, attrs);
@@ -1762,7 +1764,7 @@ unsigned long iommu_dma_get_merge_boundary(struct device *dev)
 	return (1UL << __ffs(domain->pgsize_bitmap)) - 1;
 }
 
-size_t iommu_dma_opt_mapping_size(void)
+size_t iommu_dma_max_opt_mapping_size(void)
 {
 	return iova_rcache_range();
 }
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`__ffs`
- 外部函数：`alloc_pages_node`
- 外部函数：`dma_alloc_contiguous`
- 外部函数：`dma_pgprot`
- 外部函数：`get_order`
- 外部函数：`iova_rcache_range`
- 大写宏：`NULL`
- 大写宏：`PAGE_KERNEL`
- 外部类型：`pgprot_t`
- 外部类型：`size_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-a065975ec6` → 本草稿移入 `cases/defect/auto-linux-a065975ec6/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
