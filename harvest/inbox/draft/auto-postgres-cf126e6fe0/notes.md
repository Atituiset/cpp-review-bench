# auto-postgres-cf126e6fe0

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#6d4607e886a083f7cddb121a84f6d8ee23addd6d](https://github.com/postgres/postgres/commit/6d4607e886a083f7cddb121a84f6d8ee23addd6d) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-10-14 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 32 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #6d4607e886a083f7cddb121a84f6d8ee23addd6d (https://github.com/postgres/postgres/commit/6d4607e886a083f7cddb121a84f6d8ee23addd6d)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: None（原始 PR diff 行 None；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 6d4607e886a083f7cddb121a84f6d8ee23addd6d Release memory allocated by ndistinct_for_combination() :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -28,6 +28,7 @@
 #include "catalog/pg_statistic_ext.h"
 #include "catalog/pg_statistic_ext_data.h"
 #include "statistics/extended_stats_internal.h"
+#include "utils/memutils.h"
 #include "utils/syscache.h"
 #include "utils/typcache.h"
 #include "varatt.h"
@@ -89,13 +90,18 @@ statext_ndistinct_build(double totalrows, StatsBuildData *data)
 	uint32		itemcnt;
 	int			numattrs = data->nattnums;
 	int			numcombs = num_combinations(numattrs);
+	MemoryContext cxt;
 
 	result = palloc(offsetof(MVNDistinct, items) +
 					numcombs * sizeof(MVNDistinctItem));
 	result->magic = STATS_NDISTINCT_MAGIC;
 	result->type = STATS_NDISTINCT_TYPE_BASIC;
 	result->nitems = numcombs;
 
+	cxt = AllocSetContextCreate(CurrentMemoryContext,
+								"ndistinct_for_combination cxt",
+								ALLOCSET_DEFAULT_SIZES);
+
 	itemcnt = 0;
 	for (k = 2; k <= numattrs; k++)
 	{
@@ -109,6 +115,7 @@ statext_ndistinct_build(double totalrows, StatsBuildData *data)
 		{
 			MVNDistinctItem *item = &result->items[itemcnt];
 			int			j;
+			MemoryContext oldcxt;
 
 			item->attributes = palloc_array(AttrNumber, k);
 			item->nattributes = k;
@@ -121,8 +128,12 @@ statext_ndistinct_build(double totalrows, StatsBuildData *data)
 				Assert(AttributeNumberIsValid(item->attributes[j]));
 			}
 
+			/* release memory used by ndistinct calculation */
+			oldcxt = MemoryContextSwitchTo(cxt);
 			item->ndistinct =
 				ndistinct_for_combination(totalrows, data, k, combination);
+			MemoryContextSwitchTo(oldcxt);
+			MemoryContextReset(cxt);
 
 			itemcnt++;
 			Assert(itemcnt <= result->nitems);
@@ -131,6 +142,8 @@ statext_ndistinct_build(double totalrows, StatsBuildData *data)
 		generator_free(generator);
 	}
 
+	MemoryContextDelete(cxt);
+
 	/* must consume exactly the whole output array */
 	Assert(itemcnt == result->nitems);
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`combinations`
- 外部函数：`elog`
- 外部函数：`floor`
- 外部函数：`lookup_type_cache`
- 外部函数：`multi_sort_add_dimension`
- 外部函数：`multi_sort_compare`
- 外部函数：`multi_sort_init`
- 外部函数：`offsetof`
- 外部函数：`palloc`
- 外部函数：`palloc0_array`
- 外部函数：`palloc_array`
- 外部函数：`pfree`
- 外部函数：`qsort_interruptible`
- 大写宏：`ERROR`
- 大写宏：`STATS_NDISTINCT_MAGIC`
- 大写宏：`STATS_NDISTINCT_TYPE_BASIC`
- 大写宏：`TYPECACHE_LT_OPR`
- 外部类型：`AttrNumber`
- 外部类型：`Clamp`
- 外部类型：`CombinationGenerator`
- 外部类型：`Datum`
- 外部类型：`For`
- 外部类型：`In`
- 外部类型：`InvalidOid`
- 外部类型：`MVNDistinct`
- 外部类型：`MVNDistinctItem`
- 外部类型：`MultiSortSupport`
- 外部类型：`Oid`
- 外部类型：`SortItem`
- 外部类型：`StatsBuildData`
- 外部类型：`TypeCacheEntry`
- 外部类型：`VacAttrStats`
- 外部类型：`We`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-cf126e6fe0` → 本草稿移入 `cases/defect/auto-postgres-cf126e6fe0/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
