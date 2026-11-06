# auto-linux-ab35ba438d

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#eedaa7d61f1ec8ae053a9ece0d553785350ac486](https://github.com/torvalds/linux/commit/eedaa7d61f1ec8ae053a9ece0d553785350ac486) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-06 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 20 |
| 编译错误数（gcc syntax-only） | 2（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #eedaa7d61f1ec8ae053a9ece0d553785350ac486 (https://github.com/torvalds/linux/commit/eedaa7d61f1ec8ae053a9ece0d553785350ac486)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 161；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT eedaa7d61f1ec8ae053a9ece0d553785350ac486 Merge tag 'chrome-platform-firmware-fixes-v7.4-rc2' of git://git.kernel.org/pub/scm/linux/kernel/git/chrome-platform/lin :: PR 修复动作推断：修复前越界访问（加边界检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -114,6 +114,30 @@ void coreboot_driver_unregister(struct coreboot_driver *driver)
 }
 EXPORT_SYMBOL(coreboot_driver_unregister);
 
+/*
+ * Whether the memory of a CBMEM entry may be mapped: it must be exclusively
+ * System RAM, or exclusively Reserved memory, as CBMEM often is on x86, or
+ * memory that the kernel doesn't know about at all. coreboot leaves CBMEM out
+ * of the /memory node of the device tree that it passes to FIT payloads, e.g.
+ * on arm64, and only lists it in the memory reservation block.
+ *
+ * Anything else, such as no-map regions on arm64, which are IORESOURCE_MEM
+ * (see request_standard_resources() in arch/arm64/kernel/setup.c), must not
+ * be mapped.
+ */
+static bool cbmem_entry_is_mappable(const struct lb_cbmem_entry *entry)
+{
+	resource_size_t start = entry->address;
+	size_t size = entry->entry_size;
+
+	return region_intersects(start, size, IORESOURCE_SYSTEM_RAM,
+				 IORES_DESC_NONE) == REGION_INTERSECTS ||
+	       region_intersects(start, size, IORESOURCE_MEM,
+				 IORES_DESC_RESERVED) == REGION_INTERSECTS ||
+	       region_intersects(start, size, IORESOURCE_MEM,
+				 IORES_DESC_NONE) == REGION_DISJOINT;
+}
+
 static int coreboot_table_populate(struct device *dev, void *ptr, resource_size_t len)
 {
 	int i, ret;
@@ -149,23 +173,7 @@ static int coreboot_table_populate(struct device *dev, void *ptr, resource_size_
 
 		switch (device->entry.tag) {
 		case LB_TAG_CBMEM_ENTRY:
-			/*
-			 * Skip entries that are not exclusively System RAM or
-			 * Reserved memory.
-			 * On ARM64, no-map regions are filtered out as they are
-			 * IORESOURCE_MEM (see request_standard_resources() in
-			 * arch/arm64/kernel/setup.c).
-			 * On x86, CBMEM often resides in standard reserved regions
-			 * (IORES_DESC_RESERVED).
-			 */
-			if (region_intersects(device->cbmem_entry.address,
-					      device->cbmem_entry.entry_size,
-					      IORESOURCE_SYSTEM_RAM,
-					      IORES_DESC_NONE) != REGION_INTERSECTS &&
-			    region_intersects(device->cbmem_entry.address,
-					      device->cbmem_entry.entry_size,
-					      IORESOURCE_MEM,
-					      IORES_DESC_RESERVED) != REGION_INTERSECTS) {
+			if (!cbmem_entry_is_mappable(&device->cbmem_entry)) {
 				kfree(device);
 				continue;
 			}
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`driver_unregister`
- 外部函数：`kfree`
- 外部函数：`region_intersects`
- 大写宏：`ARM64`
- 大写宏：`CBMEM`
- 大写宏：`EXPORT_SYMBOL`
- 大写宏：`IORESOURCE_MEM`
- 大写宏：`IORESOURCE_SYSTEM_RAM`
- 大写宏：`IORES_DESC_NONE`
- 大写宏：`IORES_DESC_RESERVED`
- 大写宏：`LB_TAG_CBMEM_ENTRY`
- 大写宏：`RAM`
- 大写宏：`REGION_INTERSECTS`
- 外部类型：`On`
- 外部类型：`Reserved`
- 外部类型：`Skip`
- 外部类型：`System`
- 外部类型：`coreboot_driver`
- 外部类型：`device`
- 外部类型：`resource_size_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-ab35ba438d` → 本草稿移入 `cases/defect/auto-linux-ab35ba438d/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
