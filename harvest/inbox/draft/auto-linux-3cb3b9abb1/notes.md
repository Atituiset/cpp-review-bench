# auto-linux-3cb3b9abb1

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#0f2732620d6ec49d3e6c06831575eaec6bc99d42](https://github.com/torvalds/linux/commit/0f2732620d6ec49d3e6c06831575eaec6bc99d42) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-10-08 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 69 |
| 编译错误数（gcc syntax-only） | 56（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #0f2732620d6ec49d3e6c06831575eaec6bc99d42 (https://github.com/torvalds/linux/commit/0f2732620d6ec49d3e6c06831575eaec6bc99d42)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 6（原始 PR diff 行 2724；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 0f2732620d6ec49d3e6c06831575eaec6bc99d42 Merge tag 'for-7.3/dm-fixes-2' of git://git.kernel.org/pub/scm/linux/kernel/git/device-mapper/linux-dm :: PR 修复动作推断：修复前缺判空即解引用（短路保护 if(ptr && ptr->...)）

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -180,6 +180,7 @@ struct dm_integrity_c {
 	struct dm_bufio_client *bufio;
 	struct workqueue_struct *metadata_wq;
 	struct superblock *sb;
+	struct superblock *sb_copy;
 	unsigned int journal_pages;
 	unsigned int n_bitmap_blocks;
 
@@ -2721,7 +2722,7 @@ static void dm_integrity_inline_recheck(struct work_struct *w)
 	outgoing_data = dio->integrity_payload + PAGE_SIZE;
 
 	while (dio->bio_details.bi_iter.bi_size) {
-		char digest[HASH_MAX_DIGESTSIZE];
+		char digest[MAX_T(size_t, HASH_MAX_DIGESTSIZE, MAX_TAG_SIZE)];
 		int r;
 		struct bio_integrity_payload *bip;
 		struct bio_vec bv;
@@ -2788,7 +2789,7 @@ static inline bool dm_integrity_check(struct dm_integrity_c *ic, struct dm_integ
 	unsigned pos = 0;
 
 	while (dio->bio_details.bi_iter.bi_size) {
-		char digest[HASH_MAX_DIGESTSIZE];
+		char digest[MAX_T(size_t, HASH_MAX_DIGESTSIZE, MAX_TAG_SIZE)];
 		struct bio_vec bv = bio_iter_iovec(bio, dio->bio_details.bi_iter);
 		char *mem = integrity_kmap(ic, bv.bv_page);
 		integrity_sector_checksum(ic, &dio->ahash_req, dio->bio_details.bi_iter.bi_sector, mem, bv.bv_offset, digest);
@@ -3858,6 +3859,35 @@ static void dm_integrity_postsuspend(struct dm_target *ti)
 	ic->journal_uptodate = true;
 }
 
+/*
+ * The superblock is re-read from the device on every resume, so that we pick
+ * up the flags and the recalculate position. The geometry described by the
+ * superblock must not change though - the in-memory structures (and the
+ * journal in particular) were sized according to the superblock that was
+ * validated in the constructor. Reject a superblock that was modified behind
+ * our back.
+ *
+ * Only the fields that the driver never rewrites may be tested here. In
+ * particular, "version" is recalculated by sb_set_version on every superblock
+ * write and it depends on SB_FLAG_RECALCULATING and SB_FLAG_DIRTY_BITMAP, and
+ * SB_FLAG_DISCARD_KEYED may be set by dm_integrity_resume itself.
+ */
+static bool superblock_changed(struct dm_integrity_c *ic)
+{
+	const __le32 immutable_flags = cpu_to_le32(SB_FLAG_HAVE_JOURNAL_MAC |
+						   SB_FLAG_FIXED_PADDING |
+						   SB_FLAG_FIXED_HMAC |
+						   SB_FLAG_INLINE);
+
+	return memcmp(ic->sb->magic, ic->sb_copy->magic, sizeof(ic->sb->magic)) != 0 ||
+	       ic->sb->log2_interleave_sectors != ic->sb_copy->log2_interleave_sectors ||
+	       ic->sb->integrity_tag_size != ic->sb_copy->integrity_tag_size ||
+	       ic->sb->journal_sections != ic->sb_copy->journal_sections ||
+	       ic->sb->log2_sectors_per_block != ic->sb_copy->log2_sectors_per_block ||
+	       ((ic->sb->flags ^ ic->sb_copy->flags) & immutable_flags) != 0 ||
+	       memcmp(ic->sb->salt, ic->sb_copy->salt, SALT_SIZE) != 0;
+}
+
 static void dm_integrity_resume(struct dm_target *ti)
 {
 	struct dm_integrity_c *ic = ti->private;
@@ -3876,6 +3906,18 @@ static void dm_integrity_resume(struct dm_target *ti)
 	if (r)
 		dm_integrity_io_error(ic, "reading superblock", r);
 
+	if (unlikely(superblock_changed(ic))) {
+		/*
+		 * Restore the superblock that we validated in the constructor,
+		 * so that the rest of the driver doesn't operate on values
+		 * that don't match the in-memory structures.
+		 */
+		memcpy(ic->sb, ic->sb_copy, sizeof(struct superblock));
+		DMERR("The superblock was changed while the device was suspended");
+		dm_integrity_io_error(ic, "superblock check", -EINVAL);
+		goto skip_writes;
+	}
+
 	if (ic->mode == 'R')
 		goto skip_writes;
 
@@ -5427,6 +5469,13 @@ static int dm_integrity_ctr(struct dm_target *ti, unsigned int argc, char **argv
 		ic->just_formatted = true;
 	}
 
+	ic->sb_copy = kmemdup(ic->sb, sizeof(struct superblock), GFP_KERNEL);
+	if (!ic->sb_copy) {
+		ti->error = "Cannot allocate superblock copy";
+		r = -ENOMEM;
+		goto bad;
+	}
+
 	if (!ic->meta_dev && ic->mode != 'I') {
 		r = dm_set_target_max_io_len(ti, 1U << ic->sb->log2_interleave_sectors);
 		if (r)
@@ -5525,6 +5574,7 @@ static void dm_integrity_dtr(struct dm_target *ti)
 	kvfree(ic->journal_tree);
 	if (ic->sb)
 		
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`ahash_request_set_callback`
- 外部函数：`ahash_request_set_crypt`
- 外部函数：`ahash_request_set_tfm`
- 外部函数：`atomic64_inc`
- 外部函数：`bio_iter_iovec`
- 外部函数：`cmpxchg`
- 外部函数：`cpu_to_le32`
- 外部函数：`cpu_to_le64`
- 外部函数：`crypto_ahash_digest`
- 外部函数：`crypto_free_shash`
- 外部函数：`crypto_shash_final`
- 外部函数：`crypto_shash_init`
- 外部函数：`crypto_shash_update`
- 外部函数：`crypto_wait_req`
- 外部函数：`dm_set_target_max_io_len`
- 外部函数：`free_pages_exact`
- 外部函数：`get_random_bytes`
- 外部函数：`is_vmalloc_addr`
- 外部函数：`kmap_local_page`
- 外部函数：`kvfree`
- 外部函数：`likely`
- 外部函数：`mempool_alloc`
- 外部函数：`memset`
- 外部函数：`might_sleep`
- 外部函数：`offset_in_page`
- 外部函数：`sg_init_table`
- 外部函数：`sg_set_buf`
- 外部函数：`sg_set_page`
- 外部函数：`unlikely`
- 外部函数：`vmalloc_to_page`
- 大写宏：`CRYPTO_TFM_REQ_MAY_SLEEP`
- 大写宏：`DECLARE_CRYPTO_WAIT`
- 大写宏：`DMERR`
- 大写宏：`EILSEQ`
- 大写宏：`GFP_NOIO`
- 大写宏：`HASH_MAX_DIGESTSIZE`
- 大写宏：`NULL`
- 大写宏：`PAGE_SIZE`
- 大写宏：`SECTOR_SHIFT`
- 大写宏：`SHASH_DESC_ON_STACK`
- 外部类型：`Error`
- 外部类型：`ahash_request`
- 外部类型：`atomic64_t`
- 外部类型：`bio_integrity_payload`
- 外部类型：`bio_list`
- 外部类型：`bio_set`
- 外部类型：`bio_vec`
- 外部类型：`completion`
- 外部类型：`crypto_ahash`
- 外部类型：`crypto_shash`
- 外部类型：`crypto_skcipher`
- 外部类型：`delayed_work`
- 外部类型：`dm_bufio_client`
- 外部类型：`dm_dev`
- 外部类型：`dm_io_client`
- 外部类型：`dm_target`
- 外部类型：`list_head`
- 外部类型：`mempool_t`
- 外部类型：`notifier_block`
- 外部类型：`page`
- 外部类型：`page_list`
- 外部类型：`rb_node`
- 外部类型：`rb_root`
- 外部类型：`scatterlist`
- 外部类型：`sector_t`
- 外部类型：`skcipher_request`
- 外部类型：`spinlock_t`
- 外部类型：`timer_list`
- 外部类型：`wait_queue_head_t`
- 外部类型：`work_struct`
- 外部类型：`workqueue_struct`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-3cb3b9abb1` → 本草稿移入 `cases/defect/auto-linux-3cb3b9abb1/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
