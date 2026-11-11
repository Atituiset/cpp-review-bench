# auto-linux-35a6e670a1

> 本文件是**移植 blueprint**：draft 不是半成品用例，accept = 承诺参照真实案例移植重写一个可编译用例。

## 溯源

| 项 | 值 |
|---|---|
| 源仓 | torvalds/linux |
| 源 PR | [#5f5be6584cf5af0d70ee2fd40aa6264da18617ec](https://github.com/torvalds/linux/commit/5f5be6584cf5af0d70ee2fd40aa6264da18617ec) |
| 许可证 | GPL-2.0 |
| 移植策略 | rewrite（只允许参考，必须重写表达） |
| 采集时间 | 2026-11-11 |
| track 方向 | defect 候选（polarity=must_find） |
| 外部依赖数（dep_count） | 29 |
| 编译错误数（gcc syntax-only） | 22（0=切片已达编译地板） |

- 采集工具: pr-mining（GitHub 已合并 fix-PR 爬取）
- 采集信号: 标题/修复 diff 含缺陷特征（fix/leak/overflow/null/...）
- 源 PR: #5f5be6584cf5af0d70ee2fd40aa6264da18617ec (https://github.com/torvalds/linux/commit/5f5be6584cf5af0d70ee2fd40aa6264da18617ec)
- 候选初判 scenario: **cwe-476（候选猜测，待 LLM/人审定，非真值）**
- 候选初判锚点行: 4（原始 PR diff 行 470；PR 修复前的代码，待确认是否为 bug）

## 缺陷描述与触发条件

COMMIT 5f5be6584cf5af0d70ee2fd40aa6264da18617ec Merge tag 'vfs-7.4-rc3.fixes' of git://git.kernel.org/pub/scm/linux/kernel/git/vfs/vfs :: 标题含缺陷信号（fix/leak/overflow/...），未从 diff 定位修复动作

- 触发条件（一句话复述，移植者补写）：

> 移植者须知：accept 前必须能用一句话复述触发条件，并在本文件补写

## 真实修复 diff（PR 改了什么）

```diff
@@ -467,10 +467,6 @@ static int blkdev_iomap_begin(struct inode *inode, loff_t offset, loff_t length,
 
 static DEFINE_IOMAP_ITER_NEXT(blkdev_iomap_next, blkdev_iomap_begin);
 
-static const struct iomap_ops blkdev_iomap_ops = {
-	.iomap_next		= blkdev_iomap_next,
-};
-
 #ifdef CONFIG_BUFFER_HEAD
 static int blkdev_get_block(struct inode *inode, sector_t iblock,
 		struct buffer_head *bh, int create)
@@ -524,13 +520,13 @@ const struct address_space_operations def_blk_aops = {
 #else /* CONFIG_BUFFER_HEAD */
 static int blkdev_read_folio(struct file *file, struct folio *folio)
 {
-	iomap_bio_read_folio(folio, &blkdev_iomap_ops);
+	iomap_bio_read_folio(folio, blkdev_iomap_next);
 	return 0;
 }
 
 static void blkdev_readahead(struct readahead_control *rac)
 {
-	iomap_bio_readahead(rac, &blkdev_iomap_ops);
+	iomap_bio_readahead(rac, blkdev_iomap_next);
 }
 
 static ssize_t blkdev_writeback_range(struct iomap_writepage_ctx *wpc,
@@ -736,7 +732,7 @@ static ssize_t blkdev_buffered_write(struct kiocb *iocb, struct iov_iter *from)
 	 * i_blkbits/folio order and truncating the page cache.
 	 */
 	inode_lock_shared(inode);
-	ret = iomap_file_buffered_write(iocb, from, &blkdev_iomap_ops, NULL,
+	ret = iomap_file_buffered_write(iocb, from, blkdev_iomap_next, NULL,
 					NULL);
 	inode_unlock_shared(inode);
 	return ret;
```

## 移植要点

before 切片依赖的外部符号（启发式粗判，移植时需补桩/声明）：

- 外部函数：`bdev_logical_block_size`
- 外部函数：`block_read_full_folio`
- 外部函数：`i_size_read`
- 外部函数：`inode_lock_shared`
- 外部函数：`inode_unlock_shared`
- 外部函数：`iomap_add_to_ioend`
- 外部函数：`iomap_bio_read_folio`
- 外部函数：`iomap_bio_readahead`
- 外部函数：`iomap_file_buffered_write`
- 大写宏：`ALIGN_DOWN`
- 大写宏：`CONFIG_BUFFER_HEAD`
- 大写宏：`DEFINE_IOMAP_ITER_NEXT`
- 大写宏：`EIO`
- 大写宏：`IOMAP_F_BUFFER_HEAD`
- 大写宏：`IOMAP_MAPPED`
- 大写宏：`IOMAP_WRITE`
- 大写宏：`I_BDEV`
- 大写宏：`NULL`
- 大写宏：`WARN_ON_ONCE`
- 外部类型：`block_device`
- 外部类型：`buffer_head`
- 外部类型：`file`
- 外部类型：`folio`
- 外部类型：`inode`
- 外部类型：`iomap`
- 外部类型：`iomap_ops`
- 外部类型：`iomap_writepage_ctx`
- 外部类型：`loff_t`
- 外部类型：`readahead_control`
- 外部类型：`sector_t`
- 外部类型：`ssize_t`

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

1. 完成上面检查清单后评论 `/case accept auto-linux-35a6e670a1` → 本草稿移入 `cases/defect/auto-linux-35a6e670a1/`（五文件齐备）
2. `ci.yml` 在 PR 合并后对该 case 跑 9 工具（KLEE/CodeQL/Infer/CSA/CppCheck/clang-tidy/CodeChecker/Joern/Cooddy）
3. `tools/eval.py` 对照 golden 判四态（PASS/FN/FP/EXTRA），回流到报告
4. 正式仓由你手动触发 LLM 评审（agent-reviewer）定 scenario 真值，写入 golden
