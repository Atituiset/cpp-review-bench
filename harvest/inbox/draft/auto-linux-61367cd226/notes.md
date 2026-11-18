# auto-linux-61367cd226

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#63fdb511c27afcebce797e3db580b3534ddfa639](https://github.com/torvalds/linux/commit/63fdb511c27afcebce797e3db580b3534ddfa639) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-18 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 70 |
| 编译错误数（gcc syntax-only） | 134（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #63fdb511c27afcebce797e3db580b3534ddfa639 (https://github.com/torvalds/linux/commit/63fdb511c27afcebce797e3db580b3534ddfa639)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 862；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 63fdb511c27afcebce797e3db580b3534ddfa639 Merge tag 'mm-core-7.4-rc4' of git://git.kernel.org/pub/scm/linux/kernel/git/mm/core :: merged fix-PR（默认候选，待 LLM/人审定真值）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -781,6 +781,7 @@ struct pfn_reader_user {
 	struct file *file;
 	struct folio **ufolios;
 	size_t ufolios_len;
+	unsigned long ufolios_count;
 	unsigned long ufolios_offset;
 	struct folio **ufolios_next;
 };
@@ -800,6 +801,7 @@ static void pfn_reader_user_init(struct pfn_reader_user *user,
 	user->file = (pages->type == IOPT_ADDRESS_FILE) ? pages->file : NULL;
 	user->ufolios = NULL;
 	user->ufolios_len = 0;
+	user->ufolios_count = 0;
 	user->ufolios_next = NULL;
 	user->ufolios_offset = 0;
 }
@@ -843,6 +845,7 @@ static long pin_memfd_pages(struct pfn_reader_user *user, unsigned long start,
 
 	offset >>= PAGE_SHIFT;
 	user->ufolios_next = user->ufolios;
+	user->ufolios_count = nfolios;
 	user->ufolios_offset = offset;
 
 	for (i = 0; i < nfolios; i++) {
@@ -859,8 +862,12 @@ static long pin_memfd_pages(struct pfn_reader_user *user, unsigned long start,
 			} else {
 				int rc = folio_add_pins(folio, npin - 1);
 
-				if (rc)
+				if (rc) {
+					unpin_user_pages(user->upages,
+							 upages - user->upages);
+					unpin_folios(user->ufolios + i, nfolios - i);
 					return rc;
+				}
 
 				while (npin--)
 					*upages++ = folio_page(folio, offset++);
@@ -1251,14 +1258,14 @@ static int pfn_reader_next(struct pfn_reader *pfns)
 			return -EINVAL;
 
 		rc = pfn_reader_fill_span(pfns);
+		pfns->batch_end_index = pfns->batch_start_index +
+					pfns->batch.total_pfns;
 		if (rc)
 			return rc;
 
 		if (WARN_ON(!pfns->batch.total_pfns))
 			return -EINVAL;
 
-		pfns->batch_end_index =
-			pfns->batch_start_index + pfns->batch.total_pfns;
 		if (pfns->batch_end_index == pfns->span.last_used + 1)
 			interval_tree_double_span_iter_next(&pfns->span);
 
@@ -1320,7 +1327,7 @@ static void pfn_reader_release_pins(struct pfn_reader *pfns)
 		if (!user->file) {
 			unpin_user_pages(user->upages + start_index, npages);
 		} else {
-			long n = user->ufolios_len / sizeof(*user->ufolios);
+			const unsigned long n = user->ufolios_count;
 
 			unpin_folios(user->ufolios_next,
 				     user->ufolios + n - user->ufolios_next);
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`check_add_overflow`
- 外部函数：`container_of`
- 外部函数：`folio_add_pins`
- 外部函数：`folio_nr_pages`
- 外部函数：`folio_page`
- 外部函数：`interval_tree_iter_first`
- 外部函数：`interval_tree_span_iter_advance`
- 外部函数：`interval_tree_span_iter_done`
- 外部函数：`iommu_iova_to_phys`
- 外部函数：`iommufd_should_fail`
- 外部函数：`iopt_area_index`
- 外部函数：`iopt_area_iova`
- 外部函数：`iopt_area_last_index`
- 外部函数：`iopt_is_dmabuf`
- 外部函数：`kmalloc`
- 外部函数：`memfd_pin_folios`
- 外部函数：`min`
- 外部函数：`min_t`
- 外部函数：`mmap_read_lock`
- 外部函数：`mmget`
- 外部函数：`mmget_not_zero`
- 外部函数：`page_to_pfn`
- 外部函数：`pfn_reader_destroy`
- 外部函数：`pin_user_pages_fast`
- 外部函数：`pin_user_pages_remote`
- 外部函数：`rcu_read_lock`
- 外部函数：`rcu_read_unlock`
- 外部函数：`unpin_folios`
- 外部函数：`unpin_user_pages`
- 外部函数：`xa_is_value`
- 外部函数：`xa_to_value`
- 外部函数：`xas_next`
- 外部函数：`xas_retry`
- 大写宏：`ARRAY_SIZE`
- 大写宏：`BATCH_CPU_MEMORY`
- 大写宏：`BATCH_MMIO`
- 大写宏：`CONFIG_IOMMUFD_TEST`
- 大写宏：`CPU`
- 大写宏：`EFAULT`
- 大写宏：`EINVAL`
- 大写宏：`ENOMEM`
- 大写宏：`GFP_KERNEL`
- 大写宏：`IOPT_ADDRESS_FILE`
- 大写宏：`IS_ENABLED`
- 大写宏：`MAX_NPFNS`
- 大写宏：`NULL`
- 大写宏：`PAGE_SHIFT`
- 大写宏：`PAGE_SIZE`
- 大写宏：`PHYS_PFN`
- 大写宏：`ULONG_MAX`
- 大写宏：`WARN_ON`
- 大写宏：`XA_STATE`
- 外部类型：`All`
- 外部类型：`Always`
- 外部类型：`If`
- 外部类型：`One`
- 外部类型：`PFNs`
- 外部类型：`Pull`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`file`
- 外部类型：`folio`
- 外部类型：`interval_tree_double_span_iter`
- 外部类型：`interval_tree_node`
- 外部类型：`iommu_domain`
- 外部类型：`iopt_area`
- 外部类型：`iopt_pages`
- 外部类型：`page`
- 外部类型：`phys_addr_t`
- 外部类型：`phys_vec`
- 外部类型：`size_t`
- 外部类型：`uintptr_t`
- 外部类型：`xarray`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-61367cd226` → 本草稿移入 `cases/defect/auto-linux-61367cd226/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
