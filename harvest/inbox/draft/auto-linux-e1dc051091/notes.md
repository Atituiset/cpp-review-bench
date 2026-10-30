# auto-linux-e1dc051091

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#0ff8f40110872ecd3a9918e56ddd0ff1b46fb10e](https://github.com/torvalds/linux/commit/0ff8f40110872ecd3a9918e56ddd0ff1b46fb10e) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-30 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 50 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #0ff8f40110872ecd3a9918e56ddd0ff1b46fb10e (https://github.com/torvalds/linux/commit/0ff8f40110872ecd3a9918e56ddd0ff1b46fb10e)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 1953；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 0ff8f40110872ecd3a9918e56ddd0ff1b46fb10e Merge tag 'ntfs3_for_7.4' of https://github.com/Paragon-Software-Group/linux-ntfs3 :: PR 修复动作推断：修复前缺判空即解引用（加 null 检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -13,6 +13,7 @@
 #include "debug.h"
 #include "ntfs.h"
 #include "ntfs_fs.h"
+#include <trace/events/ntfs3.h>
 
 /*
  * You can set external NTFS_MIN_LOG2_OF_CLUMP/NTFS_MAX_LOG2_OF_CLUMP to manage
@@ -61,11 +62,16 @@ static int attr_load_runs(struct ATTRIB *attr, struct ntfs_inode *ni,
 			  struct runs_tree *run, const CLST *vcn)
 {
 	int err;
-	CLST svcn = le64_to_cpu(attr->nres.svcn);
-	CLST evcn = le64_to_cpu(attr->nres.evcn);
+	CLST svcn, evcn;
 	u32 asize;
 	u16 run_off;
 
+	if (!attr->non_res)
+		return -EIO;
+
+	svcn = le64_to_cpu(attr->nres.svcn);
+	evcn = le64_to_cpu(attr->nres.evcn);
+
 	if (svcn >= evcn + 1 || run_is_mapped_full(run, svcn, evcn))
 		return 0;
 
@@ -167,6 +173,8 @@ int attr_allocate_clusters(struct ntfs_sb_info *sbi, struct runs_tree *run,
 	CLST flen, vcn0 = vcn, pre = pre_alloc ? *pre_alloc : 0;
 	size_t cnt = run->count;
 
+	trace_ntfs3_attr_allocate_clusters(sbi->sb, vcn, lcn, len, opt);
+
 	for (;;) {
 		err = ntfs_look_for_free_space(sbi, lcn, len + pre, &lcn, &flen,
 					       opt);
@@ -253,8 +261,8 @@ int attr_allocate_clusters(struct ntfs_sb_info *sbi, struct runs_tree *run,
  */
 int attr_make_nonresident(struct ntfs_inode *ni, struct ATTRIB *attr,
 			  struct ATTR_LIST_ENTRY *le, struct mft_inode *mi,
-			  u64 new_size, struct runs_tree *run,
-			  struct ATTRIB **ins_attr, struct page *page)
+			  struct runs_tree *run, struct ATTRIB **ins_attr,
+			  struct page *page)
 {
 	struct ntfs_sb_info *sbi;
 	struct ATTRIB *attr_s;
@@ -394,13 +402,15 @@ static int attr_set_size_res(struct ntfs_inode *ni, struct ATTRIB *attr,
 	char *next = Add2Ptr(attr, asize);
 	s64 dsize = ALIGN(new_size, 8) - ALIGN(rsize, 8);
 
+	if (new_size > sbi->record_size ||
+	    (dsize > 0 && used + dsize > sbi->max_bytes_per_attr)) {
+		return attr_make_nonresident(ni, attr, le, mi, run, ins_attr,
+					     NULL);
+	}
+
 	if (dsize < 0) {
 		memmove(next + dsize, next, tail);
 	} else if (dsize > 0) {
-		if (used + dsize > sbi->max_bytes_per_attr)
-			return attr_make_nonresident(ni, attr, le, mi, new_size,
-						     run, ins_attr, NULL);
-
 		memmove(next + dsize, next, tail);
 		memset(next, 0, dsize);
 	}
@@ -463,6 +473,12 @@ int attr_set_size_ex(struct ntfs_inode *ni, enum ATTR_TYPE type,
 		goto bad_inode;
 	}
 
+	trace_ntfs3_attr_set_size_ex(&ni->vfs_inode, le32_to_cpu(type),
+				     attr_b->non_res ?
+					     le64_to_cpu(attr_b->nres.data_size) :
+					     le32_to_cpu(attr_b->res.data_size),
+				     new_size, keep_prealloc, no_da);
+
 	if (!attr_b->non_res) {
 		err = attr_set_size_res(ni, attr_b, le_b, mi_b, new_size, run,
 					&attr_b);
@@ -556,6 +572,10 @@ int attr_set_size_ex(struct ntfs_inode *ni, enum ATTR_TYPE type,
 		}
 
 next_le_1:
+		if (!attr->non_res) {
+			err = -EIO;
+			goto out;
+		}
 		svcn = le64_to_cpu(attr->nres.svcn);
 		evcn = le64_to_cpu(attr->nres.evcn);
 	}
@@ -960,6 +980,9 @@ int attr_data_get_block(struct ntfs_inode *ni, CLST vcn, CLST clen, CLST *lcn,
 	if (res)
 		*res = NULL;
 
+	trace_ntfs3_attr_data_get_block(&ni->vfs_inode, vcn, clen, new, zero,
+					no_da);
+
 	/* Try to find in cache. */
 	down_read(&ni->file.run_lock);
 	if (run_lookup_entry_da(&ni->file.run, !no_da ? &ni->file.run_da : NULL,
@@ -1008,11 +1031,8 @@ int attr_data_get_block_locked(struct ntfs_inode *ni, CLST vcn, CLST clen,
 	int step;
 
 again:
-	if (run_lookup_entry_da(run, da ? &ni->file.run_da : NULL, vcn, lcn,
-				len)) {
-	} else {
+	if (!run_lookup_entry_da(run, da ? run_da : NULL, vcn, lcn, len))
 		*len = 0;
-	}
 
 	if (*len) {
 		if (*lcn != SPARSE_LCN || !new)
@@ -1034,6 +1054,21 @@ int attr_data_get_block_locked(struct ntfs_inode *ni, CLST vcn, CLST clen,
 
 	if (!attr_b->non_res) {
 		u32 data_size = le32_to_cpu(attr_b->res.data_size);
+
+		/*
+		 * A resident attribute is copied into a single page below
+		 * (alloc_page() + memcpy()) and mapped as a one-page
+		 * IOMAP_INLINE extent by ntfs_iomap_begin().  mi_enum_attr()
+		 * only bounds the resident value length against t
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`al_remove_le`
- 外部函数：`bytes_to_cluster`
- 外部函数：`cpu_to_le32`
- 外部函数：`cpu_to_le64`
- 外部函数：`down_read`
- 外部函数：`le16_to_cpu`
- 外部函数：`le32_to_cpu`
- 外部函数：`le64_to_cpu`
- 外部函数：`memmove`
- 外部函数：`memset`
- 外部函数：`mi_find_attr`
- 外部函数：`mi_pack_runs`
- 外部函数：`mi_remove_attr`
- 外部函数：`ni_find_attr`
- 外部函数：`ni_insert_nonresident`
- 外部函数：`ni_load_mi`
- 外部函数：`ntfs_look_for_free_space`
- 外部函数：`resident_data`
- 外部函数：`run_is_mapped_full`
- 外部函数：`run_lookup_entry`
- 外部函数：`run_lookup_entry_da`
- 外部函数：`run_truncate_around`
- 外部函数：`run_unpack_ex`
- 大写宏：`ALIGN`
- 大写宏：`ATTRIB`
- 大写宏：`ATTR_DATA`
- 大写宏：`ATTR_LIST_ENTRY`
- 大写宏：`CLST`
- 大写宏：`EINVAL`
- 大写宏：`ENOENT`
- 大写宏：`MFT_REC`
- 大写宏：`NTFS_MAX_LOG2_OF_CLUMP`
- 大写宏：`NTFS_MIN_LOG2_OF_CLUMP`
- 大写宏：`NULL`
- 大写宏：`RESIDENT_LCN`
- 大写宏：`SPARSE_LCN`
- 外部类型：`ATTRIB`
- 外部类型：`ATTR_LIST_ENTRY`
- 外部类型：`Check`
- 外部类型：`Last`
- 外部类型：`MFT_REC`
- 外部类型：`Remove`
- 外部类型：`Try`
- 外部类型：`You`
- 外部类型：`mft_inode`
- 外部类型：`ntfs_inode`
- 外部类型：`ntfs_sb_info`
- 外部类型：`page`
- 外部类型：`runs_tree`
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

1. 完成上面检查清单后评论 `/case accept auto-linux-e1dc051091` → 本草稿移入 `cases/defect/auto-linux-e1dc051091/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
