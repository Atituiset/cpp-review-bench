# auto-postgres-98e1d61052

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | postgres/postgres |
| 源 PR | [#6a3da13d376f5950fd105eedddc4a1e829e6b398](https://github.com/postgres/postgres/commit/6a3da13d376f5950fd105eedddc4a1e829e6b398) |
| 许可证 | PostgreSQL |
| 移植策略 | direct（宽松许可，可直接移植） |
| 采集时间 | 2026-11-10 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 60 |
| 编译错误数（gcc syntax-only） | 82（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #6a3da13d376f5950fd105eedddc4a1e829e6b398 (https://github.com/postgres/postgres/commit/6a3da13d376f5950fd105eedddc4a1e829e6b398)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 199；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 6a3da13d376f5950fd105eedddc4a1e829e6b398 Move per-attribute version to pg_attribute.attversion :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -154,55 +154,6 @@ verify_compact_attribute(TupleDesc tupdesc, int attnum)
 #endif
 }
 
-/*
- * Attribute versions, stored after the attribute arrays in the back branches.
- * versions[0] is the aggregate version, versions[1..natts] are the
- * per-attribute versions.  See "Version storage in tuple descriptors" in
- * src/backend/catalog/README.attver.md.
- */
-typedef struct TupleDescVersionData
-{
-	uint64		magic;			/* TUPLEDESC_VERSION_MAGIC */
-	XLogRecPtr	versions[FLEXIBLE_ARRAY_MEMBER];
-} TupleDescVersionData;
-
-#define TUPLEDESC_VERSION_MAGIC UINT64CONST(0xDE5C00000001DE5C)
-
-/* size of the version storage for a descriptor with natts attributes */
-static inline Size
-TupleDescVersionDataSize(int natts)
-{
-	return offsetof(TupleDescVersionData, versions) +
-		sizeof(XLogRecPtr) * (natts + 1);
-}
-
-static inline TupleDescVersionData *
-TupleDescGetVersionData(TupleDesc tupdesc)
-{
-	Size		off = offsetof(struct TupleDescData, compact_attrs) +
-		tupdesc->natts * sizeof(CompactAttribute) +
-		tupdesc->natts * sizeof(FormData_pg_attribute);
-
-	return (TupleDescVersionData *) ((char *) tupdesc + MAXALIGN(off));
-}
-
-/*
- * Address of the attribute version array.  A descriptor not allocated by
- * CreateTemplateTupleDesc() doesn't have one, which the magic number
- * detects.
- */
-static inline XLogRecPtr *
-TupleDescVersionArray(TupleDesc tupdesc)
-{
-	TupleDescVersionData *vdata = TupleDescGetVersionData(tupdesc);
-
-	if (unlikely(vdata->magic != TUPLEDESC_VERSION_MAGIC))
-		elog(ERROR, "tuple descriptor for type %u lacks attribute versions",
-			 tupdesc->tdtypeid);
-
-	return vdata->versions;
-}
-
 /*
  * CreateTemplateTupleDesc
  *		This function allocates an empty tuple descriptor structure.
@@ -214,7 +165,6 @@ TupleDesc
 CreateTemplateTupleDesc(int natts)
 {
 	TupleDesc	desc;
-	TupleDescVersionData *vdata;
 	size_t		sz;
 
 	/*
@@ -239,9 +189,6 @@ CreateTemplateTupleDesc(int natts)
 		natts * sizeof(CompactAttribute) +
 		natts * sizeof(FormData_pg_attribute);
 
-	/* Storage for TupleDescVersionData */
-	sz = MAXALIGN(sz) + TupleDescVersionDataSize(natts);
-
 	desc = (TupleDesc) palloc(sz);
 
 	/*
@@ -254,13 +201,11 @@ CreateTemplateTupleDesc(int natts)
 	desc->tdrefcount = -1;		/* assume not reference-counted */
 
 	/*
-	 * Zero-initialize the per-attribute and aggregate versions. RECORD
-	 * descriptors aren't versioned, and we don't know if this is going to be
-	 * used for something else, so we have to zero it here.
+	 * Zero-initialize the aggregate version. RECORD descriptors aren't
+	 * versioned, and we don't know if this is going to be used for something
+	 * else, so we have to zero it here.
 	 */
-	vdata = TupleDescGetVersionData(desc);
-	memset(vdata, 0, TupleDescVersionDataSize(natts));
-	vdata->magic = TUPLEDESC_VERSION_MAGIC;
+	TupleDescSetVersion(desc, InvalidXLogRecPtr);
 
 	/* This will be set to the correct value by TupleDescFinalize() */
 	desc->firstNonCachedOffsetAttr = -1;
@@ -296,31 +241,6 @@ CreateTupleDesc(int natts, Form_pg_attribute *attrs)
 	return desc;
 }
 
-/*
- * CopyTupleDescVersions
- *		Copy the attribute versions from src into dst.
- *
- * dst may have fewer attributes than src (a truncated copy), in which case
- * the aggregate version becomes that of dst's last attribute, preserving the
- * invariant that the aggregate is the last attribute's version.
- */
-static void
-CopyTupleDescVersions(TupleDesc dst, TupleDesc src)
-{
-	Assert(src->tdtypeid != RECORDOID);
-
-	if (dst->natts == 0)
-	{
-		TupleDescSetVersion(dst, InvalidXLogRecPtr);
-		return;
-	}
-
-	for (int i = 1; i <= dst->natts; i++)
-		TupleDescSetAttrVersion(dst, i, TupleDescGetAttrVersion(src, i));
-
-	TupleDescSetVersion(dst, TupleDescGetAttrVersion(src, dst->natts));
-}
-
 /*
  * CreateTupleDescCopy
  *		This function creates a new TupleDesc by copying from an existing
@@ -363,9 +283,6 @@ CreateTupleDescCopy(TupleDesc tupdesc)
 	desc->tdtypeid = tupdesc->tdtypeid;
 	desc->tdtypmod = tupdesc->tdtypmod;
 
-	if
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`elog`
- 外部函数：`memcpy`
- 外部函数：`memset`
- 外部函数：`offsetof`
- 外部函数：`palloc`
- 外部函数：`unlikely`
- 大写宏：`ERROR`
- 大写宏：`FLEXIBLE_ARRAY_MEMBER`
- 大写宏：`MAXALIGN`
- 大写宏：`README`
- 大写宏：`RECORD`
- 大写宏：`RECORDOID`
- 大写宏：`UINT64CONST`
- 外部类型：`Address`
- 外部类型：`Attribute`
- 外部类型：`CompactAttribute`
- 外部类型：`Constraints`
- 外部类型：`Copy`
- 外部类型：`CopyTupleDescVersions`
- 外部类型：`CreateTemplateTupleDesc`
- 外部类型：`CreateTupleDescCopy`
- 外部类型：`Fill`
- 外部类型：`Flat`
- 外部类型：`FormData_pg_attribute`
- 外部类型：`Implementation`
- 外部类型：`In`
- 外部类型：`InvalidXLogRecPtr`
- 外部类型：`Return`
- 外部类型：`See`
- 外部类型：`Since`
- 外部类型：`Size`
- 外部类型：`Storage`
- 外部类型：`The`
- 外部类型：`This`
- 外部类型：`TupleDesc`
- 外部类型：`TupleDescCopySize`
- 外部类型：`TupleDescCopySized`
- 外部类型：`TupleDescData`
- 外部类型：`TupleDescVersionData`
- 外部类型：`Version`
- 外部类型：`XLogRecPtr`
- 外部类型：`Zero`
- 外部类型：`size_t`

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

1. 完成上面检查清单后评论 `/case accept auto-postgres-98e1d61052` → 本草稿移入 `cases/defect/auto-postgres-98e1d61052/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
