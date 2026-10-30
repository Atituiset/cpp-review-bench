# auto-linux-61a54f38fb

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
| 外部依赖数（dep_count） | 77 |
| 编译错误数（gcc syntax-only） | 1（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #0ff8f40110872ecd3a9918e56ddd0ff1b46fb10e (https://github.com/torvalds/linux/commit/0ff8f40110872ecd3a9918e56ddd0ff1b46fb10e)
- 候选初判 scenario: **cwe-787（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 5（原始 PR diff 行 537；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 0ff8f40110872ecd3a9918e56ddd0ff1b46fb10e Merge tag 'ntfs3_for_7.4' of https://github.com/Paragon-Software-Group/linux-ntfs3 :: PR 修复动作推断：修复前越界访问（加边界/长度检查）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -14,6 +14,7 @@
 #include "debug.h"
 #include "ntfs.h"
 #include "ntfs_fs.h"
+#include <trace/events/ntfs3.h>
 
 /* Convert little endian UTF-16 to NLS string. */
 int ntfs_utf16_to_nls(struct ntfs_sb_info *sbi, const __le16 *name, u32 len,
@@ -34,6 +35,9 @@ int ntfs_utf16_to_nls(struct ntfs_sb_info *sbi, const __le16 *name, u32 len,
 		/* UTF-16 -> UTF-8 */
 		ret = utf16s_to_utf8s((wchar_t *)name, len, UTF16_LITTLE_ENDIAN,
 				      buf, buf_len);
+        if (ret >= buf_len) {
+            ret = buf_len-1;
+        }
 		buf[ret] = '\0';
 		return ret;
 	}
@@ -290,6 +294,8 @@ struct inode *dir_search_flags(struct inode *dir, const struct cpu_str *uni,
 	struct inode *inode = NULL;
 	struct ntfs_fnd *fnd_a = NULL;
 
+	trace_ntfs3_dir_search_u(dir, uni ? uni->len : 0);
+
 	if (!fnd) {
 		fnd_a = fnd_get();
 		if (!fnd_a) {
@@ -525,45 +531,50 @@ static int ntfs_readdir(struct file *file, struct dir_context *ctx)
 	if (!name)
 		return -ENOMEM;
 
+	/*
+	 * Keep directory metadata stable for the whole walk. Loading subrecords
+	 * once is not enough if concurrent writeback can still compact ATTR_LIST
+	 * entries and free the record that ntfs_read_hdr() is currently walking.
+	 */
+	ni_lock(ni);
+
 	if (!ni->mi_loaded && ni->attr_list.size) {
 		/*
 		 * Directory inode is locked for read.
 		 * Load all subrecords to avoid 'write' access to 'ni' during
 		 * directory reading.
 		 */
-		ni_lock(ni);
-		if (!ni->mi_loaded && ni->attr_list.size) {
-			err = ni_load_all_mi(ni);
-			if (!err)
-				ni->mi_loaded = true;
-		}
-		ni_unlock(ni);
+		err = ni_load_all_mi(ni);
 		if (err)
 			goto out;
+		ni->mi_loaded = true;
 	}
 
-	/*
-	 * Keep directory metadata stable for the whole walk. Loading subrecords
-	 * once is not enough if concurrent writeback can still compact ATTR_LIST
-	 * entries and free the record that ntfs_read_hdr() is currently walking.
-	 */
-	ni_lock(ni);
-
 	root = indx_get_root(&ni->dir, ni, NULL, NULL);
 	if (!root) {
 		err = -EINVAL;
-		goto out_unlock;
+		goto out;
 	}
 
 	if (pos >= sbi->record_size) {
 		bit = (pos - sbi->record_size) >> index_bits;
 	} else {
+		/*
+		 * Release ni_lock before dir_emit (which may fault into
+		 * user pages).  root->ihdr lives in the main MFT record
+		 * whose lifetime is bound to the inode, and i_rwsem shared
+		 * prevents concurrent directory modifications.
+		 */
+		ni_unlock(ni);
+
 		/*
 		 * Add each name from root in 'ctx'.
 		 */
 		err = ntfs_read_hdr(sbi, ni, &root->ihdr, 0, pos, name, ctx);
 		if (err)
-			goto out_unlock;
+			goto out;
+
+		ni_lock(ni);
 		bit = 0;
 	}
 
@@ -574,7 +585,7 @@ static int ntfs_readdir(struct file *file, struct dir_context *ctx)
 		/* Get the next used index. */
 		err = indx_used_bit(&ni->dir, ni, &bit);
 		if (err)
-			goto out_unlock;
+			goto out;
 
 		if (bit == MINUS_ONE_T) {
 			/* no more used indexes. end of dir. */
@@ -584,13 +595,19 @@ static int ntfs_readdir(struct file *file, struct dir_context *ctx)
 		if (bit >= max_bit) {
 			/* Corrupted directory. */
 			err = -EINVAL;
-			goto out_unlock;
+			goto out;
 		}
 
 		err = indx_read_ra(&ni->dir, ni, bit << ni->dir.idx2vbn_bits,
 				   &node, &file->f_ra);
 		if (err)
-			goto out_unlock;
+			goto out;
+
+		/*
+		 * node->index is a kmalloc'd buffer, safe after ni_unlock.
+		 * Release lock before dir_emit to avoid lock-order issues.
+		 */
+		ni_unlock(ni);
 
 		/*
 		 * Add each name from index in 'ctx'.
@@ -599,13 +616,14 @@ static int ntfs_readdir(struct file *file, struct dir_context *ctx)
 				    ((u64)bit << index_bits) + sbi->record_size,
 				    pos, name, ctx);
 		if (err)
-			goto out_unlock;
+			goto out;
+
+		ni_lock(ni);
 	}
 
-out_unlock:
+out:
 	ni_unlock(ni);
 
-out:
 	kfree(name);
 	put_indx_node(node);
 
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`de_is_last`
- 外部函数：`dir_emit`
- 外部函数：`file`
- 外部函数：`fname_full_size`
- 外部函数：`fnd_get`
- 外部函数：`fs_umode_to_dtype`
- 外部函数：`hex_byte_pack`
- 外部函数：`indx_get_root`
- 外部函数：`indx_read_ra`
- 外部函数：`indx_used_bit`
- 外部函数：`ino_get`
- 外部函数：`inode`
- 外部函数：`iput`
- 外部函数：`le16_to_cpu`
- 外部函数：`le32_to_cpu`
- 外部函数：`mi_is_ref`
- 外部函数：`ni_load_all_mi`
- 外部函数：`ni_lock`
- 外部函数：`ni_unlock`
- 外部函数：`ntfs_err`
- 外部函数：`ntfs_iget5`
- 外部函数：`ntfs_is_meta_file`
- 外部函数：`ntfs_warn`
- 外部函数：`purposes`
- 外部函数：`uni2char`
- 外部函数：`utf16s_to_utf8s`
- 大写宏：`ATTR_FILE_NAME`
- 大写宏：`ATTR_LIST`
- 大写宏：`ATTR_STD`
- 大写宏：`DT_DIR`
- 大写宏：`DT_REG`
- 大写宏：`EINVAL`
- 大写宏：`ENOMEM`
- 大写宏：`FILE_ATTRIBUTE_DIRECTORY`
- 大写宏：`FILE_ATTRIBUTE_HIDDEN`
- 大写宏：`FILE_NAME_DOS`
- 大写宏：`INDEX_HDR`
- 大写宏：`IS_ERR_OR_NULL`
- 大写宏：`MFT`
- 大写宏：`MFT_REC_ROOT`
- 大写宏：`MINUS_ONE_T`
- 大写宏：`NLS`
- 大写宏：`NLS_MAX_CHARSET_SIZE`
- 大写宏：`NTFS`
- 大写宏：`NTFS_DE`
- 大写宏：`NULL`
- 大写宏：`PATH_MAX`
- 大写宏：`SIZEOF_ATTRIBUTE_FILENAME`
- 大写宏：`UTF`
- 大写宏：`UTF16_LITTLE_ENDIAN`
- 外部类型：`ATTR_FILE_NAME`
- 外部类型：`Add`
- 外部类型：`Convert`
- 外部类型：`Corrupted`
- 外部类型：`Directory`
- 外部类型：`Get`
- 外部类型：`INDEX_HDR`
- 外部类型：`It`
- 外部类型：`Keep`
- 外部类型：`Load`
- 外部类型：`Loading`
- 外部类型：`NTFS_DE`
- 外部类型：`Should`
- 外部类型：`Skip`
- 外部类型：`Submit`
- 外部类型：`The`
- 外部类型：`To`
- 外部类型：`Unfortunately`
- 外部类型：`Unless`
- 外部类型：`dir_context`
- 外部类型：`inode`
- 外部类型：`nls_table`
- 外部类型：`ntfs_fnd`
- 外部类型：`ntfs_inode`
- 外部类型：`ntfs_sb_info`
- 外部类型：`wchar_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-61a54f38fb` → 本草稿移入 `cases/defect/auto-linux-61a54f38fb/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
